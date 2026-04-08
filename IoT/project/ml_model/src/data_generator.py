import numpy as np
import pandas as pd
from datetime import datetime, timedelta
from tqdm import tqdm
from utils import DATA_DIR, NUM_GEYSERS
import os

def generate_synthetic_data(days=365):
    print(f"Generating synthetic data for {days} days...")
    
    # Time settings
    start_time = datetime(2025, 1, 1, 0, 0, 0)
    num_minutes = days * 24 * 60
    
    # Create time index
    time_idx = pd.date_range(start=start_time, periods=num_minutes, freq='min')
    
    # Extract temporal features
    df_base = pd.DataFrame({'timestamp': time_idx})
    df_base['hour'] = df_base['timestamp'].dt.hour
    df_base['minute'] = df_base['timestamp'].dt.minute
    df_base['day_of_week'] = df_base['timestamp'].dt.dayofweek
    df_base['month'] = df_base['timestamp'].dt.month
    
    # Base ambient temperature model: colder in winter (Jan=1, Dec=12), warmer in summer (May-Aug)
    # Average Indian climate roughly: Jan(15C) to May(35C) to Dec(15C)
    month_base_temp = {1: 15, 2: 18, 3: 25, 4: 30, 5: 35, 6: 34, 7: 30, 8: 28, 9: 28, 10: 25, 11: 20, 12: 16}
    
    # Base probability of usage by hour (0 to 23)
    # Peak at 7-9 AM and 6-8 PM
    hour_prob = np.array([0.01, 0.01, 0.01, 0.01, 0.01, 0.05, 0.15, 0.30, 0.30, 0.15, 0.05, 0.02,
                         0.02, 0.02, 0.02, 0.02, 0.05, 0.10, 0.25, 0.25, 0.15, 0.05, 0.02, 0.01])
    
    all_data = []
    
    for gid in range(1, NUM_GEYSERS + 1):
        print(f"Generating for Geyser {gid}...")
        df = df_base.copy()
        df['geyser_id'] = gid
        
        # Calculate ambient temp
        base_t = df['month'].map(month_base_temp).values
        # Add daily fluctuation (coldest at 4 AM, hottest at 2 PM)
        daily_var = -5 * np.cos((df['hour'].values - 4) * 2 * np.pi / 24)
        df['ambient_temp'] = base_t + daily_var + np.random.normal(0, 1, num_minutes)
        
        # Calculate usage probability (higher in winter)
        season_multiplier = 1.0 + (35 - df['ambient_temp'].values) / 35.0 # more likely to use hot water if colder
        weekend_shift = (df['day_of_week'] >= 5) * 1.5 # shift probability or just scale it
        
        # Calculate event triggers
        probs = hour_prob[df['hour'].values] * season_multiplier * (np.random.uniform(0.5, 1.5, num_minutes))
        # Turn probs into sparse events (e.g. 1% chance per minute during peak = decent amount of showers)
        probs = probs * 0.02
        event_starts = np.random.rand(num_minutes) < probs
        
        # Expand event starts to durations (5 to 15 mins)
        flow_rate = np.zeros(num_minutes)
        is_active = 0
        duration = 0
        for i in range(num_minutes):
            if is_active > 0:
                flow_rate[i] = np.random.uniform(3.0, 7.0) # L/min
                is_active -= 1
            elif event_starts[i]:
                is_active = int(np.random.uniform(5, 15))
                flow_rate[i] = np.random.uniform(3.0, 7.0)
                
        df['flow_rate'] = flow_rate
        
        # Thermodynamic simulation (baseline: basic thermostat keeps water hot)
        water_temp = np.zeros(num_minutes)
        is_heating = np.zeros(num_minutes)
        current_temp = df['ambient_temp'].values[0]
        heating_state = 0
        
        ambient_arr = df['ambient_temp'].values
        flow_arr = df['flow_rate'].values
        
        for i in range(num_minutes):
            # Heating logic: basic thermostat ON < 45C, OFF > 60C
            if current_temp < 45:
                heating_state = 1
            elif current_temp > 60:
                heating_state = 0
                
            is_heating[i] = heating_state
            
            # Temp update
            # 1. Heating adds ~1.5 C per minute (2000W / 25L)
            if heating_state == 1:
                current_temp += 1.5
            
            # 2. Flow removes hot water and adds ambient water
            if flow_arr[i] > 0:
                # Weighted average of remaining hot water and incoming cold water
                mix_ratio = flow_arr[i] / 25.0 # Max capacity 25L
                current_temp = (current_temp * (1 - mix_ratio)) + (ambient_arr[i] * mix_ratio)
            else:
                # 3. Cooling curve (standby loss) ~ 0.05 C per min
                current_temp -= (current_temp - ambient_arr[i]) * 0.001
                
            water_temp[i] = current_temp
            
        df['water_temp'] = water_temp
        df['is_heating'] = is_heating
        df['power_consumption'] = is_heating * 2000
        
        df['water_level'] = 100 # Assuming always full
        
        # Target Variables:
        # demand: 1 if flow_rate > 0 in the next 15 minutes
        # demand_volume: sum of flow_rate in next 15 minutes
        df['demand_volume'] = df['flow_rate'].rolling(window=15).sum().shift(-15).fillna(0)
        df['demand'] = (df['demand_volume'] > 0).astype(int)
        
        all_data.append(df)
        
    final_df = pd.concat(all_data, ignore_index=True)
    
    # Save
    out_path = os.path.join(DATA_DIR, 'synthetic_hostel_data.csv')
    print(f"Saving to {out_path}...")
    final_df.to_csv(out_path, index=False)
    print("Done! Data shape:", final_df.shape)

if __name__ == "__main__":
    generate_synthetic_data(days=365)
