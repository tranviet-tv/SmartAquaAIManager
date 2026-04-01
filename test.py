"""
Smart Aquaculture AI Evaluation Script
This script tests the accuracy of the trained weights on the dataset.
Ensure you update the weights (w_fan, b_fan, w_pump, b_pump) with the outputs from train-ai.py.
"""
import pandas as pd
import numpy as np

def load_data(csv_path):
    """Loads and returns cleaned dataset, filtering temperature bounds."""
    try:
        df = pd.read_csv(csv_path)
        # Filter valid temperatures between 0 and 100
        df_clean = df[(df['TEMPERATURE'] > 0) & (df['TEMPERATURE'] < 100)].copy()
        return df_clean
    except FileNotFoundError:
        print(f"Error: File '{csv_path}' not found!")
        return None

def evaluate_predictions(y_real, y_pred, name):
    """Calculates and prints the accuracy of predictions."""
    accuracy = np.mean(y_real == y_pred) * 100
    print(f"{name} Accuracy: {accuracy:.2f}%")
    return accuracy

def main():
    df_clean = load_data('IoTPond10.csv')
    if df_clean is None:
        return

    # --- 1. SET TRAINED WEIGHTS HERE ---
    # Copy these values from the output of the train-ai.py script
    w_fan, b_fan = 4.380426, -1.459521
    w_pump, b_pump = 1.003667, -0.008972

    # --- 2. TEST FAN (Temperature) ---
    X_temp = df_clean['TEMPERATURE'].values
    X_temp_norm = (X_temp - 20) / (40 - 20)  # Normalize to 20-40 range
    
    y_fan_real = (X_temp > 28).astype(int)   # Ground truth: > 28 is ON (1)
    
    # AI Prediction
    y_fan_pred_val = w_fan * X_temp_norm + b_fan
    y_fan_pred = (y_fan_pred_val > 0.5).astype(int) # Decision threshold 0.5

    # --- 3. TEST PUMP (Turbidity) ---
    X_turb = df_clean['TURBIDITY'].values
    X_turb_norm = (X_turb - (-13)) / (100 - (-13))  # Normalize to -13 to 100 range
    
    y_pump_real = (X_turb > 50).astype(int)         # Ground truth: > 50 is ON (1)
    
    # AI Prediction
    y_pump_pred_val = w_pump * X_turb_norm + b_pump
    y_pump_pred = (y_pump_pred_val > 0.5).astype(int)

    # --- 4. PRINT REPORT ---
    print("\n--- AI EVALUATION REPORT ---")
    evaluate_predictions(y_fan_real, y_fan_pred, "FAN")
    evaluate_predictions(y_pump_real, y_pump_pred, "PUMP")

    # Show 5 random samples for a quick comparison insight
    print("\nComparing 5 Random Samples (0: OFF, 1: ON):")
    samples = df_clean.sample(5)
    
    for i, row in samples.iterrows():
        t = row['TEMPERATURE']
        tu = row['TURBIDITY']
        
        # Quick AI Calculation directly with formula
        f_ai = 1 if (w_fan * ((t - 20) / 20) + b_fan) > 0.5 else 0
        p_ai = 1 if (w_pump * ((tu + 13) / 113) + b_pump) > 0.5 else 0
        
        real_f = 1 if t > 28 else 0
        real_p = 1 if tu > 50 else 0
        
        print(f"Temp: {t:5.1f}C -> Real: {real_f}, AI: {f_ai} | "
              f"Turb: {tu:5.1f} NTU -> Real: {real_p}, AI: {p_ai}")

if __name__ == "__main__":
    main()