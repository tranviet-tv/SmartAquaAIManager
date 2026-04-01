"""
Smart Aquaculture AI Training Script
This script trains a linear regression model to predict when to turn on the fan (based on temperature) 
and the pump (based on turbidity).
It then uploads the trained weights and biases to Firebase Realtime Database.
"""
import pandas as pd
from sklearn.linear_model import LinearRegression
import pyrebase
from datetime import datetime
import os

# --- 1. FIREBASE CONFIGURATION ---
# Replace these values or use environment variables.
# DO NOT hardcode your actual API keys in the public repository!
config = {
    "apiKey": os.getenv("FIREBASE_API_KEY", "YOUR_API_KEY"),
    "authDomain": os.getenv("FIREBASE_AUTH_DOMAIN", "YOUR_PROJECT.firebaseapp.com"),
    "databaseURL": os.getenv("FIREBASE_DATABASE_URL", "https://YOUR_PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app"),
    "storageBucket": os.getenv("FIREBASE_STORAGE_BUCKET", "YOUR_PROJECT.firebasestorage.app")
}

def init_firebase():
    """Initializes and returns the Firebase database reference."""
    try:
        firebase = pyrebase.initialize_app(config)
        return firebase.database()
    except Exception as e:
        print(f"Error initializing Firebase: {e}")
        return None

def load_and_clean_data(csv_path):
    """Loads dataset and filters out invalid temperature values."""
    try:
        df = pd.read_csv(csv_path)
        print("--- Data loaded successfully ---")
        # Keep only valid temperature readings
        return df[df['TEMPERATURE'] > 0].copy()
    except FileNotFoundError:
        print(f"Error: File '{csv_path}' not found!")
        return None

def train_model(X_raw, y_target, norm_min, norm_max):
    """
    Normalizes input and trains a linear regression model.
    Returns: The trained model, normalized input array.
    """
    X_norm = (X_raw - norm_min) / (norm_max - norm_min)
    y_flattened = y_target.astype(int).flatten()
    model = LinearRegression().fit(X_norm, y_flattened)
    return model

def main():
    db = init_firebase()
    if not db:
        print("Could not connect to Firebase. Proceeding with local training only.")
    
    df_clean = load_and_clean_data('IoTPond10.csv')
    if df_clean is None:
        return

    # --- 2. TRAINING AI ---
    
    # Train FAN model (Feature: Temperature)
    # Fan turns on when Temperature > 29C
    X_temp_raw = df_clean[['TEMPERATURE']].values
    y_fan = X_temp_raw > 29
    model_fan = train_model(X_temp_raw, y_fan, norm_min=20, norm_max=40)
    w_f, b_f = model_fan.coef_[0], model_fan.intercept_

    # Train PUMP model (Feature: Turbidity)
    # Pump turns on when Turbidity > 50 NTU
    X_turb_raw = df_clean[['TURBIDITY']].values
    y_pump = X_turb_raw > 50
    model_pump = train_model(X_turb_raw, y_pump, norm_min=-13, norm_max=100)
    w_p, b_p = model_pump.coef_[0], model_pump.intercept_

    # --- 3. UPLOAD DATA TO FIREBASE ---
    data_to_upload = {
        "w_fan": float(w_f),
        "b_fan": float(b_f),
        "w_pump": float(w_p),
        "b_pump": float(b_p),
        "last_update": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    }

    print("\n" + "="*30)
    print(f"FAN:  w = {w_f:.6f}, b = {b_f:.6f}")
    print(f"PUMP: w = {w_p:.6f}, b = {b_p:.6f}")
    print("="*30)

    if db:
        try:
            # Push to "AI_Model" node on Realtime Database
            db.child("AI_Model").set(data_to_upload)
            print("FIREBASE UPDATE SUCCESSFUL!")
        except Exception as e:
            print(f"Error uploading data to Firebase: {e}")

if __name__ == "__main__":
    main()