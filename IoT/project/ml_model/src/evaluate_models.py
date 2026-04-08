import pandas as pd
import numpy as np
import os
import joblib
import json
import matplotlib.pyplot as plt
import seaborn as sns
from utils import DATA_DIR, MODELS_DIR, PLOTS_DIR

def evaluate_models():
    print("Loading data and model...")
    data_path = os.path.join(DATA_DIR, 'processed_features.csv')
    df = pd.read_csv(data_path)
    
    model_path = os.path.join(MODELS_DIR, 'xgboost_model.pkl')
    xgb = joblib.load(model_path)
    
    # Take a 7-day slice for Geyser 1 for simulation
    # 7 days * 24 h * 60 min = 10080 rows
    df_sim = df[(df['geyser_id'] == 1)].head(10080).copy()
    
    features = [
        'hour_sin', 'hour_cos', 'day_sin', 'day_cos', 'month_sin', 'month_cos', 
        'is_weekend', 'is_peak_morning', 'is_peak_evening', 
        'ambient_temp', 'water_temp', 'water_temp_lag_15m', 'water_temp_lag_1h',
        'flow_rolling_mean_15m', 'flow_rolling_mean_1h'
    ]
    
    # Get ML predictions for the period
    df_sim['ml_predict'] = xgb.predict(df_sim[features])
    
    # Strategy 1: Always ON (Maintains 75C) -> Power is drawn whenever temp < 75
    # Simplified: Assume standby heater turns on 5 mins every hour to maintain.
    # We will approximate energy based on a standard metric: 
    # Traditional geyser kept always ON consumes ~2.5 kWh / day in standby + actual usage
    
    # Let's simulate the 3 strategies step-by-step
    energy_sim = {
        'Traditional (Always ON)': 0,
        'Basic Thermostat': 0,
        'Smart ML Predictor': 0
    }
    
    # 2000W = 33.33 Watt-hours per minute = 0.033 kWh per min ON
    KWH_PER_MIN = 2000 / 60000
    
    for i, row in df_sim.iterrows():
        # Baseline usage heating tracking
        flow = row['flow_rate']
        
        # 1. Traditional: Always ON (Maintains high temp)
        # Thermostat at 70C. We'll simplify: 
        # Standby loss + usage loss
        if flow > 0:
            energy_sim['Traditional (Always ON)'] += KWH_PER_MIN * 1.5 # Heats up faster
        else:
            energy_sim['Traditional (Always ON)'] += KWH_PER_MIN * 0.1 # 10% duty cycle for standby
            
        # 2. Basic Thermostat: This is what the base simulation used (is_heating column)
        # Maintained between 45-60C
        if row['is_heating'] == 1:
            energy_sim['Basic Thermostat'] += KWH_PER_MIN
            
        # 3. Smart ML: Predicts 15 mins ahead. Only heat if predicted demand OR currently using
        # Keep a buffer temp of e.g. 40C, heat to 60C only if prediction = 1
        if row['ml_predict'] == 1 or flow > 0:
            energy_sim['Smart ML Predictor'] += KWH_PER_MIN * 0.8
        else:
            energy_sim['Smart ML Predictor'] += KWH_PER_MIN * 0.02 # Minimal duty cycle just to keep from freezing
            
    print("\n--- Energy Consumption Comparison (7 Days) ---")
    for k, v in energy_sim.items():
        print(f"{k}: {v:.2f} kWh")
        
    # Save to JSON
    out_json = os.path.join(MODELS_DIR, 'model_comparison.json')
    with open(out_json, 'w') as f:
        json.dump(energy_sim, f, indent=4)
        
    # Plotting Energy
    plt.figure(figsize=(10, 6))
    colors = ['#ff6b6b', '#feca57', '#1dd1a1']
    sns.barplot(x=list(energy_sim.keys()), y=list(energy_sim.values()), palette=colors)
    plt.title('7-Day Energy Consumption by Strategy (Geyser 1)')
    plt.ylabel('Energy Consumed (kWh)')
    plt.ylim(0, max(energy_sim.values()) * 1.2)
    for i, v in enumerate(energy_sim.values()):
        plt.text(i, v + 0.5, f"{v:.1f} kWh", ha='center', fontweight='bold')
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, 'energy_comparison.png'))
    plt.close()
    
    # Plotting Usage Pattern (1 day)
    day_df = df_sim.head(1440)
    plt.figure(figsize=(12, 6))
    plt.plot(day_df['timestamp'], day_df['water_temp'], label='Water Temp (°C)', color='blue', alpha=0.5)
    plt.plot(day_df['timestamp'], day_df['flow_rate'] * 10, label='Water Flow (Scaled x10)', color='cyan')
    plt.fill_between(day_df['timestamp'], 0, 80, where=day_df['ml_predict']==1, color='green', alpha=0.2, label='ML Predicts Demand')
    plt.title('24-Hour Geyser Usage Simulation with ML Predictions')
    plt.xlabel('Time')
    plt.ylabel('Value')
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, 'usage_pattern_24h.png'))
    plt.close()

    print(f"\nPlots saved to {PLOTS_DIR}")

if __name__ == "__main__":
    evaluate_models()
