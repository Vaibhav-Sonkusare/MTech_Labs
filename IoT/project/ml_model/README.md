# Context-Aware Smart Water Geyser - ML Model Pipeline

This directory contains the Machine Learning pipeline and central Cloud server for the Smart Geyser project.

## Architecture

*   **Models:** Random Forest (Baseline) & XGBoost (Optimized).
*   **Central Server (`src/server.py`):** Subscribes to live sensor data via MQTT, performs feature engineering on-the-fly, and predicts user demand in real-time. It sends `ON/OFF` commands back to the hardware.
*   **Live Dashboard (`dashboard/`):** Real-time web UI showing geyser state, temperature, and live ML predictions. Connects directly to the MQTT broker over WebSockets.

## Directory Structure

*   `data/`: Data generated and processed.
*   `models/`: Saved `.pkl` model artifacts.
*   `plots/`: Evaluation visualizations.
*   `src/`: Evaluation and Server Python scripts.
*   `dashboard/`: HTML/JS/CSS for Web UI.

## Getting Started

1.  **Activate Virtual Environment:**
    ```bash
    source .venv/bin/activate
    ```
2.  **Run Server:**
    ```bash
    python src/server.py
    ```
3.  **Simulate Hardware Data (In a new terminal):**
    ```bash
    source .venv/bin/activate
    python src/mock_publisher.py
    ```
4.  **Open Dashboard:**
    Open `dashboard/index.html` in your web browser to view live predictions.

## Evaluation Results

In simulations (`src/evaluate_models.py`), the ML strategy optimizes heating cycles to cover peak demand efficiently while significantly cutting down the standby losses characteristic of traditional, always-on thermostats.
