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
- Identify the model of the motor from the recorded data and compute PI gains

## Files

| File / folder | Description |
|---|---|
| `ESP32_MQTT_DCmotor_New_Certificate_V1_1.mlapp` | MATLAB GUI |
| `motor_identification_app.m` | Model identification module (opened with the **Identification** button) |
| `generate_synthetic_data.m` | Creates `Synthetic_FO_test.mat`, synthetic data from a known model to verify the identification module |
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

## Identification module

Press **Identification** in the GUI, or run `motor_identification_app` in the
MATLAB Command Window.

1. **Data:** load a file saved with **Save** (or use the data passed from the
   GUI). For a step from 3 V to 9 V at t = 2 s, use *Segment start* = 1.5 s.
2. **Model:** choose the structure (first order + dead time, second order or
   state space) and press **Estimate model**. The dead time is searched in
   whole sampling periods.
3. **Validation:** load a second experiment to compare it with the model.
4. **PI design:** enter the desired closed-loop time constant lambda and press
   **Compute PI gains** (Kp = tau/(K lambda), Ki = 1/(K lambda)).

The *System Identification Toolbox* is optional (checkbox *Use tfest/ssest*);
without it the module uses its own least-squares fit.

### Verification with synthetic data

Run `generate_synthetic_data` to create `Synthetic_FO_test.mat`, generated from
the known model G(s) = 1.08/(0.16 s + 1), with steps of 3 V and 9 V and a fixed
random seed. Loading this file in the module with *Segment start* = 1.5 s gives
K = 1.080, tau = 0.158 s and theta = 0, the values of the true model.

## Authors

Luis Ricardo Rivera-Goitia  
Luis Enrique González-Jiménez  

Department of Electronics, Systems and Informatics  
ITESO – Universidad Jesuita de Guadalajara
