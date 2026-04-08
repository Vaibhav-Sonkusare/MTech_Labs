import pandas as pd
import numpy as np
import os
import joblib
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, accuracy_score
from utils import DATA_DIR, MODELS_DIR

def train_rf():
    print("Loading engineered features...")
    data_path = os.path.join(DATA_DIR, 'processed_features.csv')
    df = pd.read_csv(data_path)
    
    # Downsample for faster training (200k rows)
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
    
    # Train/Test Split
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    
    print(f"Training Random Forest on {len(X_train)} samples...")
    # Parameters to prevent overfitting and speed up training
    model = RandomForestClassifier(n_estimators=100, max_depth=10, min_samples_split=10, n_jobs=-1, random_state=42)
    model.fit(X_train, y_train)
    
    print("Evaluating Model...")
    y_pred = model.predict(X_test)
    acc = accuracy_score(y_test, y_pred)
    print(f"Random Forest Accuracy: {acc:.4f}")
    print("Classification Report:")
    print(classification_report(y_test, y_pred))
    
    # Save Model
    model_path = os.path.join(MODELS_DIR, 'random_forest_model.pkl')
    print(f"Saving model to {model_path}...")
    joblib.dump(model, model_path)
    
    # Feature Importances
    importances = pd.DataFrame({'feature': features, 'importance': model.feature_importances_})
    importances = importances.sort_values(by='importance', ascending=False)
    print("\nTop 5 Feature Importances:")
    print(importances.head(5))

if __name__ == "__main__":
    train_rf()
