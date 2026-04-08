import paho.mqtt.client as mqtt
import json
import time
import random
import datetime

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
BASE_TOPIC = "smart_geyser_206125030"

client = mqtt.Client()
print("Connecting to MQTT broker for simulation...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)

print("Starting mock publisher... (Press Ctrl+C to stop)")

temps = {i: random.uniform(40, 60) for i in range(1, 7)}
active_flow_duration = {i: 0 for i in range(1, 7)}
is_heating = {i: False for i in range(1, 7)}
sim_time = datetime.datetime.now().replace(hour=7, minute=0, second=0)

try:
    while True:
        sim_time += datetime.timedelta(minutes=1)
        
        for gid in range(1, 7):
            # Manage flow duration
            if active_flow_duration[gid] > 0:
                active_flow_duration[gid] -= 1
                flow = random.uniform(4.0, 7.0)
            else:
                flow = 0.0
                # 5% chance to start using geyser during morning (7 AM)
                if sim_time.hour in [7, 8] and random.random() < 0.15:
                    active_flow_duration[gid] = random.randint(5, 15)

            # Calculate dynamic ambient temperature based on month and hour
            import math
            base_month_temp = 15 + ((sim_time.month % 7) * 3) # Varies from ~15 to 33 depending on month
            daily_var = -5 * math.cos((sim_time.hour - 4) * 2 * math.pi / 24)
            # Add some minute-by-minute randomness to ambient
            ambient = base_month_temp + daily_var + random.uniform(-0.5, 0.5)

            # Thermostat simulation to prevent geysers from getting stuck at 15C
            if temps[gid] < 40:
                is_heating[gid] = True
            elif temps[gid] > 65:
                is_heating[gid] = False
                
            # Temp dynamics
            if is_heating[gid]:
                temps[gid] += 1.5  # Heating up
            if flow > 0:
                # Water loss + cold water mixing
                temps[gid] -= flow * 0.3
            else:
                # Standby cooling loss depends on delta with ambient
                temps[gid] -= (temps[gid] - ambient) * 0.005
                
            temps[gid] = max(10, min(75, temps[gid]))
                
            payload = {
                "geyser_id": gid,
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
            
            topic = f"{BASE_TOPIC}/geyser/{gid}/data"
            client.publish(topic, json.dumps(payload))
            print(f"[{sim_time.strftime('%H:%M')}] Published to {topic}: T={temps[gid]:.1f}C, Flow={flow:.1f}")
            
        time.sleep(1) # accelerate time: 1 second = 1 minute in simulation

except KeyboardInterrupt:
    print("Publisher stopped.")
    client.disconnect()
