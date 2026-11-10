# MATLAB GUI – DC Motor Control Interface

This folder contains the MATLAB App Designer interface used to control and
monitor the ESP32-based DC motor platform.

The GUI allows users to:

- Send control signals (Step, Sine, PID – continuous or discrete)
- Configure control parameters and update the PID gains during an experiment
- Configure the digital low-pass filters for speed and current
- Visualize real-time motor signals
- Save the experiment data to a `.mat` file
- Communicate with the ESP32 firmware using MQTT or TCP/IP

## Files

| File / folder | Description |
|---|---|
| `ESP32_MQTT_DCmotor_New_Certificate_V1_1.mlapp` | MATLAB GUI |
| `isrgrootx1.pem` | Root certificate for HiveMQ Cloud (TLS) |
| `*.png`, `*.jpg` | Images (equations and symbols) shown by the GUI |

The GUI loads the certificate and the images from **the same folder as the
`.mlapp`**, so keep all these files together in one folder.

## Requirements

- MATLAB R2024b or newer, with App Designer
- **Industrial Communication Toolbox** (provides `mqttclient`)
- Internet connection (for MQTT communication)

## Setup

1. Download this whole folder (keep the `.mlapp`, `isrgrootx1.pem` and the images together).
2. Optional: add the folder to the MATLAB path
   (**Home → Set Path → Add Folder… → Save**) so the images also appear in Design View.
3. Open the `.mlapp` in App Designer, go to **Code View** and, in the
   **USER SETTINGS** block of the properties, replace the placeholders (`YOUR_...`):
   `brokerAddress`, `Username`, `Password` (your MQTT broker data).
4. Do not rename the `.mlapp` file outside MATLAB; the class name must match the
   file name. Use **Save As** in App Designer instead.

## How to Run

1. Open MATLAB and the `.mlapp` file in App Designer.
2. Click **Run**.
3. Select the connection type (**MQTT Broker** or **TCP/IP**) and press **Connect**.
   - TCP/IP: first connect the computer to the ESP32 hotspot.
   - macOS: allow MATLAB in **System Settings → Privacy & Security → Local Network**,
     otherwise the TCP connection fails with "No route to host".
4. Choose the tab (Step, Sine or PID), set the parameters and press **Start**.

## Saved data

Files saved with **Save** contain the table `data` with the columns

```
Time, Speed, FiltSpeed, Current, FiltCurrent, Voltage, Error, DutyCycle
```

## Authors

Luis Ricardo Rivera-Goitia  
Luis Enrique González-Jiménez  

Department of Electronics, Systems and Informatics  
ITESO – Universidad Jesuita de Guadalajara
