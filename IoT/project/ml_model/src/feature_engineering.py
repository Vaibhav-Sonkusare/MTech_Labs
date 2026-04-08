import pandas as pd
import numpy as np
import os
from utils import DATA_DIR

def engineer_features():
    input_path = os.path.join(DATA_DIR, 'synthetic_hostel_data.csv')
    output_path = os.path.join(DATA_DIR, 'processed_features.csv')
    
    print("Loading raw data...")
    df = pd.read_csv(input_path)
    
    print("Engineering temporal features...")
    # Cyclical encoding for hour, minute, day, month
    df['hour_sin'] = np.sin(2 * np.pi * df['hour']/24)
    df['hour_cos'] = np.cos(2 * np.pi * df['hour']/24)
    
    df['day_sin'] = np.sin(2 * np.pi * df['day_of_week']/7)
    df['day_cos'] = np.cos(2 * np.pi * df['day_of_week']/7)
    
    df['month_sin'] = np.sin(2 * np.pi * df['month']/12)
    df['month_cos'] = np.cos(2 * np.pi * df['month']/12)
    
    df['is_weekend'] = (df['day_of_week'] >= 5).astype(int)
    
    # Peak hour indicators (Morning: 6-9, Evening: 18-21)
    df['is_peak_morning'] = ((df['hour'] >= 6) & (df['hour'] <= 9)).astype(int)
    df['is_peak_evening'] = ((df['hour'] >= 18) & (df['hour'] <= 21)).astype(int)
    
    print("Engineering lag and rolling features...")
    # Process per geyser to avoid leaking rows between geysers
    all_processed = []
    
    for gid in df['geyser_id'].unique():
        print(f"Processing Geyser {gid}...")
        gdf = df[df['geyser_id'] == gid].sort_values('timestamp').copy()
        
        # Lag features
        gdf['water_temp_lag_15m'] = gdf['water_temp'].shift(15).fillna(gdf['water_temp'])
        gdf['water_temp_lag_1h'] = gdf['water_temp'].shift(60).fillna(gdf['water_temp'])
        
        # Calculate recent usage
        gdf['flow_rolling_mean_15m'] = gdf['flow_rate'].rolling(15).mean().fillna(0)
        gdf['flow_rolling_mean_1h'] = gdf['flow_rate'].rolling(60).mean().fillna(0)
        
        # Target is already prepared by data_generator (demand & demand_volume)
        # Drop nan rows resulting from shift (first hour)
        gdf = gdf.dropna()
        
        all_processed.append(gdf)
        
    final_df = pd.concat(all_processed, ignore_index=True)
    
    # Downsample if needed to make training faster, use ~100k samples for training
    # But let's keep the full file for now since it's CSV
    print("Saving processed features...")
    final_df.to_csv(output_path, index=False)
    print("Done! Output shape:", final_df.shape)

if __name__ == "__main__":
    engineer_features()
