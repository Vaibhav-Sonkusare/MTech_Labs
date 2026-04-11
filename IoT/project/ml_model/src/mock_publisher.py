import paho.mqtt.client as mqtt
import json
import time
import random
import datetime
import math
import uuid

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
BASE_TOPIC = "smart_geyser"

# Simulate 2 geysers (matching hardware availability)
NUM_SIMULATED = 2

# Generate fake MAC addresses for each simulated geyser
fake_macs = {i: f"AA:BB:CC:DD:EE:{i:02X}" for i in range(1, NUM_SIMULATED + 1)}

client = mqtt.Client()

# State tracking
assigned_ids = {}       # mac -> geyser_id assigned by server
init_complete = {}      # mac -> True/False

def on_connect(client, userdata, flags, rc):
    print(f"Mock publisher connected (rc={rc})")
    # Subscribe to config responses for all our simulated MACs
    for gid, mac in fake_macs.items():
        config_topic = f"{BASE_TOPIC}/config/{mac}"
        client.subscribe(config_topic)
        print(f"Subscribed to {config_topic}")

def on_message(client, userdata, msg):
    """Handle config responses from the server."""
    try:
        payload = json.loads(msg.payload.decode())
        geyser_id = payload.get("geyser_id")
        data_topic = payload.get("data_topic")
        
        # Find which MAC this config is for
        for gid, mac in fake_macs.items():
            expected_topic = f"{BASE_TOPIC}/config/{mac}"
            if msg.topic == expected_topic:
                assigned_ids[mac] = geyser_id
                init_complete[mac] = True
                print(f"[INIT OK] Sim geyser {gid} (MAC={mac}) assigned ID={geyser_id}, Topic={data_topic}")
                break
    except Exception as e:
        print(f"Error handling config: {e}")

client.on_connect = on_connect
client.on_message = on_message

print("Connecting to MQTT broker for simulation...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()  # Non-blocking background loop

# ── PHASE 1: Initialization ──────────────────────────────────────────────────
print(f"\n--- Phase 1: Device Initialization ({NUM_SIMULATED} geysers) ---")
for gid, mac in fake_macs.items():
    init_payload = {
        "mac_address": mac,
        "info": {
            "chip_model": "ESP32-D0WD-V3",
            "chip_revision": 301,
            "cores": 2,
            "firmware_version": "1.0.0"
        }
    }
    client.publish(f"{BASE_TOPIC}/init", json.dumps(init_payload))
    print(f"[INIT] Sent registration for Sim Geyser {gid} (MAC={mac})")

# Wait for all configs to come back
print("Waiting for server to assign IDs...")
timeout = 10
start = time.time()
while len(init_complete) < NUM_SIMULATED and (time.time() - start) < timeout:
    time.sleep(0.5)

if len(init_complete) < NUM_SIMULATED:
    print(f"WARNING: Only {len(init_complete)}/{NUM_SIMULATED} geysers received config. Proceeding anyway.")
else:
    print(f"All {NUM_SIMULATED} geysers initialized successfully!")

# Build a map of sim_gid -> server-assigned ID
id_map = {}
for gid, mac in fake_macs.items():
    if mac in assigned_ids:
        id_map[gid] = assigned_ids[mac]
    else:
        id_map[gid] = gid  # fallback

print(f"ID Map: {id_map}")

# ── PHASE 2: Data Transmission ───────────────────────────────────────────────
print(f"\n--- Phase 2: Data Transmission (Press Ctrl+C to stop) ---")

temps = {gid: random.uniform(40, 60) for gid in range(1, NUM_SIMULATED + 1)}
active_flow_duration = {gid: 0 for gid in range(1, NUM_SIMULATED + 1)}
is_heating = {gid: False for gid in range(1, NUM_SIMULATED + 1)}
sim_time = datetime.datetime.now().replace(hour=7, minute=0, second=0)

try:
    while True:
        sim_time += datetime.timedelta(minutes=1)
        
        for gid in range(1, NUM_SIMULATED + 1):
            server_id = id_map.get(gid, gid)
            
            # Manage flow duration
            if active_flow_duration[gid] > 0:
                active_flow_duration[gid] -= 1
                flow = random.uniform(4.0, 7.0)
            else:
                flow = 0.0
                if sim_time.hour in [7, 8] and random.random() < 0.15:
                    active_flow_duration[gid] = random.randint(5, 15)

            # Dynamic ambient temperature
            base_month_temp = 15 + ((sim_time.month % 7) * 3)
            daily_var = -5 * math.cos((sim_time.hour - 4) * 2 * math.pi / 24)
            ambient = base_month_temp + daily_var + random.uniform(-0.5, 0.5)

            # Thermostat simulation
            if temps[gid] < 40:
                is_heating[gid] = True
            elif temps[gid] > 65:
                is_heating[gid] = False
                
            if is_heating[gid]:
                temps[gid] += 1.5
            if flow > 0:
                temps[gid] -= flow * 0.3
            else:
                temps[gid] -= (temps[gid] - ambient) * 0.005
                
            temps[gid] = max(10, min(75, temps[gid]))
                
            payload = {
                "geyser_id": server_id,
                "water_temp": round(temps[gid], 1),
                "ambient_temp": round(ambient, 1),
                "flow_rate": round(flow, 1),
                "water_level": 100,
                "hour": sim_time.hour,
                "minute": sim_time.minute,
                "month": sim_time.month,
                "day_of_week": sim_time.weekday(),
                "timestamp": sim_time.isoformat()
            }
            
            topic = f"{BASE_TOPIC}/geyser/{server_id}/data"
            client.publish(topic, json.dumps(payload))
            print(f"[{sim_time.strftime('%H:%M')}] Geyser {server_id}: T={temps[gid]:.1f}C, Flow={flow:.1f}")
            
        time.sleep(1)

except KeyboardInterrupt:
    print("\nPublisher stopped.")
    client.loop_stop()
    client.disconnect()
