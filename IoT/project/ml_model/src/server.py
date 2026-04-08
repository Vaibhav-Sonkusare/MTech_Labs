import paho.mqtt.client as mqtt
import json
import joblib
import pandas as pd
import numpy as np
import os
from collections import deque
from utils import MODELS_DIR

# Broker details
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
BASE_TOPIC = "smart_geyser_206125030"

# Load Model
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
    # Initialize state if new
    if gid not in geyser_state:
        geyser_state[gid] = {
            'flow_history': deque([0]*60, maxlen=60),
            'temp_history': deque([data.get('water_temp', 25)]*60, maxlen=60)
        }
        
    state = geyser_state[gid]
    
    # Update state
    state['flow_history'].append(data.get('flow_rate', 0))
    state['temp_history'].append(data.get('water_temp', 25))
    
    # temporal
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
    
    df = pd.DataFrame([features])
    return df

def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    topic = f"{BASE_TOPIC}/geyser/+/data"
    client.subscribe(topic)
    print(f"Subscribed to {topic}")

def on_message(client, userdata, msg):
    try:
        topic_parts = msg.topic.split('/')
        if len(topic_parts) >= 3 and topic_parts[-1] == 'data':
            gid = topic_parts[-2]
            payload = json.loads(msg.payload.decode())
            print(f"[{gid}] Received data: T={payload.get('water_temp')}C, Flow={payload.get('flow_rate')}")
            
            if xgb_model:
                features_df = get_engineered_features(gid, payload)
                prediction = int(xgb_model.predict(features_df)[0])
                confidence = float(xgb_model.predict_proba(features_df)[0][1])
                
                # Default safety: always ON if below 40C, always OFF if above 75C
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
                cmd_topic = f"{BASE_TOPIC}/geyser/{gid}/command"
                client.publish(cmd_topic, json.dumps(resp))
                
                dashboard_topic = f"{BASE_TOPIC}/dashboard/inference"
                client.publish(dashboard_topic, json.dumps(resp))
                
                print(f"[{gid}] -> Predicted: {prediction} (Conf: {confidence:.2f}) -> Action: {command}")
                
    except Exception as e:
        print(f"Error processing message: {e}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

try:
    print(f"Connecting to broker {MQTT_BROKER}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_forever()
except KeyboardInterrupt:
    print("Server stopped.")
    client.disconnect()
