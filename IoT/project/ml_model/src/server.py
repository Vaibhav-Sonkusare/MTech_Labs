import paho.mqtt.client as mqtt
import json
import joblib
import pandas as pd
import numpy as np
import os
from collections import deque
from utils import MODELS_DIR, BASE_DIR

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
    if gid not in geyser_state:
        geyser_state[gid] = {
            'flow_history': deque([0]*60, maxlen=60),
            'temp_history': deque([data.get('water_temp', 25)]*60, maxlen=60)
        }
        
    state = geyser_state[gid]
    state['flow_history'].append(data.get('flow_rate', 0))
    state['temp_history'].append(data.get('water_temp', 25))
    
    hr = data.get('hour', 0)
    month = data.get('month', 1)
    day = data.get('day_of_week', 0)
    
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
        'ambient_temp': data.get('ambient_temp', 25),
        'water_temp': data.get('water_temp', 25),
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
    print(f"Subscribed to: {BASE_TOPIC}/init, {BASE_TOPIC}/delete, {BASE_TOPIC}/geyser/+/data")

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
        
        # ── DATA PHASE: Normal operation ─────────────────────────────
        topic_parts = topic.split('/')
        if len(topic_parts) >= 3 and topic_parts[-1] == 'data':
            gid = topic_parts[-2]
            print(f"[{gid}] Received: T={payload.get('water_temp')}C, Flow={payload.get('flow_rate')}")
            
            if xgb_model:
                features_df = get_engineered_features(gid, payload)
                prediction = int(xgb_model.predict(features_df)[0])
                confidence = float(xgb_model.predict_proba(features_df)[0][1])
                
                # Safety logic
                w_temp = payload.get('water_temp', 25)
                command = "OFF"
                if w_temp < 40:
                    command = "ON"
                elif w_temp > 75:
                    command = "OFF"
                elif prediction == 1 or payload.get('flow_rate', 0) > 0:
                    command = "ON"
                
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
