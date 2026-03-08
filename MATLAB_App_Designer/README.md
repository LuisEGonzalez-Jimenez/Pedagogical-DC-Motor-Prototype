# MATLAB GUI – DC Motor Control Interface

This folder contains the MATLAB App Designer interface used to control and monitor the ESP32-based DC motor platform.

The GUI allows users to:

- Send control signals (Step, Sine, PID)
- Configure control parameters
- Visualize real-time motor signals
- Communicate with the ESP32 firmware using MQTT or TCP/IP

## File

`DC_Motor_Control_GUI.mlapp`

## Requirements

- MATLAB R2022a or newer
- App Designer
- Internet connection (for MQTT communication)

## How to Run

1. Open MATLAB
2. Navigate to this folder
3. Open the file:

4. Click **Run** inside App Designer

The GUI will connect to the ESP32 firmware to control the motor and visualize signals in real time.
