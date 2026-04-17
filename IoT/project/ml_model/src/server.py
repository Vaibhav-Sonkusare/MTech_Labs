import paho.mqtt.client as mqtt
import json
import joblib
import pandas as pd
import numpy as np
import os
from collections import deque
from datetime import datetime, timedelta
from utils import MODELS_DIR, BASE_DIR

# Global simulation state
demo_time_override = None

# Overrides state tracking
# geyser_id -> {'manual_stop_until': datetime, 'hot_water_requested_at': datetime}
geyser_overrides = {}

# ==============================================================================
# Configuration
# ==============================================================================
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
BASE_TOPIC = "smart_geyser"
REGISTRY_FILE = os.path.join(BASE_DIR, 'registry.json')

# ==============================================================================
# Device Registry (MAC → ID persistence)
# ==============================================================================
def load_registry():
    """Load registry from disk. Returns dict: {"devices": {mac: {id, name, ...}}, "next_id": N}"""
    if os.path.exists(REGISTRY_FILE):
        with open(REGISTRY_FILE, 'r') as f:
            return json.load(f)
    return {"devices": {}, "next_id": 1}

def save_registry(registry):
    """Persist registry to disk."""
    with open(REGISTRY_FILE, 'w') as f:
        json.dump(registry, f, indent=2)

def register_device(registry, mac_address, device_info=None):
    """Register a new device or return existing registration."""
    if mac_address in registry["devices"]:
        entry = registry["devices"][mac_address]
        print(f"[REGISTRY] Known device: MAC={mac_address} -> Geyser {entry['id']}")
        return entry["id"], False  # existing
    
    new_id = registry["next_id"]
    registry["devices"][mac_address] = {
        "id": new_id,
        "name": f"Geyser {new_id}",
        "mac": mac_address,
        "info": device_info or {},
        "settings": {}
    }
    registry["next_id"] = new_id + 1
    save_registry(registry)
    print(f"[REGISTRY] New device registered: MAC={mac_address} -> Geyser {new_id}")
    return new_id, True  # new

def delete_device(registry, mac_address):
    """Remove a device from the registry."""
    if mac_address in registry["devices"]:
        removed = registry["devices"].pop(mac_address)
        save_registry(registry)
        print(f"[REGISTRY] Device deleted: MAC={mac_address} (was Geyser {removed['id']})")
        return True
    return False

# ==============================================================================
# ML Model
# ==============================================================================
model_path = os.path.join(MODELS_DIR, 'xgboost_model.pkl')
try:
    xgb_model = joblib.load(model_path)
    print(f"Loaded ML model from {model_path}")
except Exception as e:
    print(f"Failed to load model: {e}")
    xgb_model = None

# State memory for lag/rolling features (storing last 60 ticks per geyser)
geyser_state = {}

def get_engineered_features(gid, data):
    """Compute real-time features from incoming sensor data."""
    global demo_time_override
    
    w_temp = data.get('water_temperature', data.get('water_temp', 25))
    flow = data.get('water_flow_speed', data.get('flow_rate', 0))
    a_temp = data.get('ambient_temperature', data.get('ambient_temp', 25))
    
    if gid not in geyser_state:
        geyser_state[gid] = {
            'flow_history': deque([0]*60, maxlen=60),
            'temp_history': deque([w_temp]*60, maxlen=60)
        }
        
    state = geyser_state[gid]
    state['flow_history'].append(flow)
    state['temp_history'].append(w_temp)
    
    if demo_time_override:
        curr_time = demo_time_override
    else:
        curr_time = datetime.now()
        
    hr = data.get('hour', curr_time.hour)
    month = data.get('month', curr_time.month)
    day = data.get('day_of_week', curr_time.weekday())
    
    features = {
        'hour_sin': np.sin(2 * np.pi * hr/24),
        'hour_cos': np.cos(2 * np.pi * hr/24),
        'day_sin': np.sin(2 * np.pi * day/7),
        'day_cos': np.cos(2 * np.pi * day/7),
        'month_sin': np.sin(2 * np.pi * month/12),
        'month_cos': np.cos(2 * np.pi * month/12),
        'is_weekend': int(day >= 5),
        'is_peak_morning': int(6 <= hr <= 9),
        'is_peak_evening': int(18 <= hr <= 21),
        'ambient_temp': a_temp,
        'water_temp': w_temp,
        'water_temp_lag_15m': list(state['temp_history'])[-15],
        'water_temp_lag_1h': state['temp_history'][0],
        'flow_rolling_mean_15m': np.mean(list(state['flow_history'])[-15:]),
        'flow_rolling_mean_1h': np.mean(state['flow_history'])
    }
    
    return pd.DataFrame([features])

# ==============================================================================
# MQTT Callbacks
# ==============================================================================
registry = load_registry()
print(f"[REGISTRY] Loaded {len(registry['devices'])} registered device(s)")

def on_connect(client, userdata, flags, rc):
    print(f"Connected to broker (rc={rc})")
    # Subscribe to initialization requests
    client.subscribe(f"{BASE_TOPIC}/init")
    # Subscribe to device deletion requests
    client.subscribe(f"{BASE_TOPIC}/delete")
    # Subscribe to data from all registered geysers
    client.subscribe(f"{BASE_TOPIC}/geyser/+/data")
    # Subscribe to control topics for UI overrides
    client.subscribe(f"{BASE_TOPIC}/geyser/+/control")
    # Subscribe to demo time override
    client.subscribe(f"{BASE_TOPIC}/demo/set_time")
    print(f"Subscribed to: {BASE_TOPIC}/init, {BASE_TOPIC}/delete, {BASE_TOPIC}/geyser/+/data, {BASE_TOPIC}/geyser/+/control, {BASE_TOPIC}/demo/set_time")

def on_message(client, userdata, msg):
    global registry
    try:
        topic = msg.topic
        payload = json.loads(msg.payload.decode())
        
        # ── INIT PHASE: Device registration ──────────────────────────
        if topic == f"{BASE_TOPIC}/init":
            mac = payload.get("mac_address", "")
            if not mac:
                print("[INIT] Received init without MAC address, ignoring.")
                return
            
            device_info = payload.get("info", {})
            geyser_id, is_new = register_device(registry, mac, device_info)
            
            # Send config back to the device on its MAC-specific channel
            config_resp = {
                "geyser_id": geyser_id,
                "name": registry["devices"][mac]["name"],
                "data_topic": f"{BASE_TOPIC}/geyser/{geyser_id}/data",
                "command_topic": f"{BASE_TOPIC}/geyser/{geyser_id}/command",
                "publish_interval_s": 5
            }
            config_topic = f"{BASE_TOPIC}/config/{mac}"
            client.publish(config_topic, json.dumps(config_resp))
            print(f"[INIT] Sent config to {config_topic}: ID={geyser_id}")
            
            # Notify dashboard about the new/reconnected device
            dashboard_msg = {
                "event": "device_registered",
                "geyser_id": geyser_id,
                "name": registry["devices"][mac]["name"],
                "mac": mac,
                "is_new": is_new
            }
            client.publish(f"{BASE_TOPIC}/dashboard/devices", json.dumps(dashboard_msg))
            return
        
        # ── DELETE PHASE: Device removal ─────────────────────────────
        if topic == f"{BASE_TOPIC}/delete":
            mac = payload.get("mac_address", "")
            if delete_device(registry, mac):
                client.publish(f"{BASE_TOPIC}/dashboard/devices", json.dumps({
                    "event": "device_deleted",
                    "mac": mac
                }))
            return
        
        # ── DEMO OVERRIDE ─────────────────────────────
        if topic == f"{BASE_TOPIC}/demo/set_time":
            time_str = payload.get("time")
            global demo_time_override
            if time_str:
                try:
                    demo_time_override = datetime.fromisoformat(time_str)
                    print(f"[DEMO] Time overridden to {demo_time_override}")
                except Exception as e:
                    print(f"[DEMO] Invalid time format: {e}")
            else:
                demo_time_override = None
                print(f"[DEMO] Time override cleared. Returning to real time.")
            return

        # ── DATA AND CONTROL PHASE ───────────────────────────────────
        topic_parts = topic.split('/')
        
        if len(topic_parts) >= 3 and topic_parts[-1] == 'control':
            gid = topic_parts[-2]
            if gid not in geyser_overrides:
                geyser_overrides[gid] = {'manual_stop_until': None, 'hot_water_requested_at': None}
            
            curr_time = demo_time_override if demo_time_override else datetime.now()
            
            if payload.get('cancel'):
                geyser_overrides[gid] = {'manual_stop_until': None, 'hot_water_requested_at': None}
                print(f"[{gid}] 🛑 User cancelled all overrides.")
            elif 'stop_minutes' in payload:
                mins = payload['stop_minutes']
                geyser_overrides[gid]['hot_water_requested_at'] = None
                geyser_overrides[gid]['manual_stop_until'] = curr_time + timedelta(minutes=mins)
                print(f"[{gid}] ⏳ Manual Stop until: {geyser_overrides[gid]['manual_stop_until']}")
            elif 'request_minutes' in payload:
                mins = payload['request_minutes']
                geyser_overrides[gid]['manual_stop_until'] = None
                geyser_overrides[gid]['hot_water_requested_at'] = curr_time + timedelta(minutes=mins)
                print(f"[{gid}] ♨️ Hot Water Requested for: {geyser_overrides[gid]['hot_water_requested_at']}")
            return

        if len(topic_parts) >= 3 and topic_parts[-1] == 'data':
            gid = topic_parts[-2]
            
            # Map new payload structure
            w_temp = payload.get('water_temperature', payload.get('water_temp', 25))
            flow = payload.get('water_flow_speed', payload.get('flow_rate', 0))
            a_temp = payload.get('ambient_temperature', payload.get('ambient_temp', 25))
            hum = payload.get('humidity', 50)
            level = payload.get('water_level', 'HIGH')
            
            # Add these specific variables back into payload for the feature engineering just in case
            payload['water_temp'] = w_temp
            payload['flow_rate'] = flow
            payload['ambient_temp'] = a_temp

            print(f"[{gid}] Received: T={w_temp}C, Flow={flow}, AmbTemp={a_temp}C, Hum={hum}%, Level={level}")
            
            if xgb_model:
                features_df = get_engineered_features(gid, payload)
                prediction = int(xgb_model.predict(features_df)[0])
                confidence = float(xgb_model.predict_proba(features_df)[0][1])
                
                # Check Overrides
                if gid not in geyser_overrides:
                    geyser_overrides[gid] = {'manual_stop_until': None, 'hot_water_requested_at': None}
                
                curr_time = demo_time_override if demo_time_override else datetime.now()
                override = geyser_overrides[gid]
                
                # Priority 1: Safety
                if str(level).upper() == 'LOW' or level == 0:
                    command = "OFF"
                    print(f"[{gid}] ⚠️ SAFETY CUTOFF: Water level LOW!")
                elif w_temp > 75:
                    command = "OFF"
                # Priority 2: Manual Stop
                elif override['manual_stop_until'] and curr_time < override['manual_stop_until']:
                    command = "OFF"
                    prediction = 0 # Mask ML output for demo
                # Priority 3: Hot Water Request (Pre-heat 30 mins prior to the requested time)
                elif override['hot_water_requested_at'] and override['hot_water_requested_at'] >= curr_time:
                    time_until_request = (override['hot_water_requested_at'] - curr_time).total_seconds() / 60.0
                    if time_until_request <= 30.0 and w_temp < 60:
                        command = "ON"
                        prediction = 1 # Mask ML output
                    else:
                        command = "OFF" if w_temp >= 60 else "OFF" # Don't heat yet if outside window, unless flow happens
                        prediction = 0
                # Priority 4: ML and Flow logic
                else:
                    if override['manual_stop_until'] and curr_time >= override['manual_stop_until']:
                        override['manual_stop_until'] = None # Clear elapsed state
                    if override['hot_water_requested_at'] and curr_time > override['hot_water_requested_at']:
                        override['hot_water_requested_at'] = None # Clear elapsed state
                        
                    if w_temp < 40:
                        command = "ON"
                    elif prediction == 1 or flow > 0:
                        command = "ON"
                    else:
                        command = "OFF"
                
                resp = {
                    "geyser_id": gid,
                    "command": command,
                    "ml_prediction": prediction,
                    "confidence": round(confidence, 3)
                }
                client.publish(f"{BASE_TOPIC}/geyser/{gid}/command", json.dumps(resp))
                client.publish(f"{BASE_TOPIC}/dashboard/inference", json.dumps(resp))
                
                print(f"[{gid}] -> Predicted: {prediction} (Conf: {confidence:.2f}) -> Action: {command}")
                
    except Exception as e:
        print(f"Error processing message: {e}")

# ==============================================================================
# Main
# ==============================================================================
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    print(f"Connecting to broker {MQTT_BROKER}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()
except KeyboardInterrupt:
    print("\nServer stopped.")
    client.disconnect()
