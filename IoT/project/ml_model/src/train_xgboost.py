import pandas as pd
import numpy as np
import os
import joblib
import xgboost as xgb
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, accuracy_score
from utils import DATA_DIR, MODELS_DIR

def train_xgb():
    print("Loading engineered features...")
    data_path = os.path.join(DATA_DIR, 'processed_features.csv')
    df = pd.read_csv(data_path)
    
    # Downsample for faster training
    if len(df) > 200000:
        df = df.sample(n=200000, random_state=42)
        
    features = [
        'hour_sin', 'hour_cos', 'day_sin', 'day_cos', 'month_sin', 'month_cos', 
        'is_weekend', 'is_peak_morning', 'is_peak_evening', 
        'ambient_temp', 'water_temp', 'water_temp_lag_15m', 'water_temp_lag_1h',
        'flow_rolling_mean_15m', 'flow_rolling_mean_1h'
    ]
    
    X = df[features]
    y = df['demand'] # Binary target
    
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    # Check class imbalance
    num_positive = sum(y_train == 1)
    num_negative = sum(y_train == 0)
    scale_weight = num_negative / max(num_positive, 1)
    
    print(f"Training XGBoost on {len(X_train)} samples with scale_pos_weight={scale_weight:.2f}...")
    
    model = xgb.XGBClassifier(
        n_estimators=100, 
        max_depth=6, 
        learning_rate=0.1, 
        scale_pos_weight=scale_weight,
        random_state=42,
        n_jobs=-1
    )
    
    model.fit(X_train, y_train)
    
    print("Evaluating Model...")
    y_pred = model.predict(X_test)
    acc = accuracy_score(y_test, y_pred)
    print(f"XGBoost Accuracy: {acc:.4f}")
    print("Classification Report:")
    print(classification_report(y_test, y_pred))
    
    # Save Model
    model_path = os.path.join(MODELS_DIR, 'xgboost_model.pkl')
    print(f"Saving model to {model_path}...")
    joblib.dump(model, model_path)

if __name__ == "__main__":
    train_xgb()
