# ESP32 Firmware – DC Motor Control Platform

Firmware for the ESP32 that runs the Step, Sine and PID experiments, measures
speed (encoder) and current (INA219), applies the digital filters and sends the
data to the MATLAB GUI through MQTT or TCP/IP.

## Files

| File | Description |
|---|---|
| `ESP32_Firmware_V1_1/ESP32_Firmware_V1_1.ino` | Firmware (keep it inside a folder with the same name; Arduino IDE requires it) |
| `TFT_eSPI_config/User_Setup.h` | Pin configuration for the TFT_eSPI display library |

## Requirements

- **Arduino IDE** 2.x
- Board package **esp32 by Espressif Systems** 3.x (tested with 3.1.3), board **ESP32 Dev Module**
- Libraries (Library Manager):
  - PubSubClient (Nick O'Leary)
  - ArduinoJson (Benoit Blanchon)
  - Adafruit INA219 (install it with its dependencies)
  - TFT_eSPI (Bodmer)

## Display configuration (TFT_eSPI)

Copy `TFT_eSPI_config/User_Setup.h` to

```
Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
```

replacing the original file (keep a backup of it). Pins used:
CS 15, DC 2, RST 4, MOSI 23, SCLK 18, MISO 19, TOUCH_CS 33.

## How to Upload the Firmware

1. Open `ESP32_Firmware_V1_1/ESP32_Firmware_V1_1.ino` in Arduino IDE.
2. In the **USER SETTINGS** block replace the placeholders (`YOUR_...`):
   - `ssid`, `password`: WiFi network for MQTT mode (must be **2.4 GHz**)
   - `mqttServer`, `mqttUserName`, `mqttPwd`: your MQTT broker (e.g. HiveMQ Cloud)
3. Select **Tools → Board → ESP32 Dev Module** and the serial port.
4. Upload. If the upload fails (common on macOS): **Tools → Upload Speed → 115200**.

## Communication modes

- **MQTT**: the ESP32 joins the WiFi network and connects to the broker (TLS, port 8883).
- **TCP/IP**: the ESP32 creates its own WiFi hotspot; the computer joins it and the
  GUI connects directly to the ESP32.

## Data format

Each sample sent by the ESP32 (5 ms in continuous mode, T in discrete mode):

```
current_mA, w, time_s, Vt, w_filt, current_filt, duty, e
```

## Digital filters

First-order IIR low-pass filters obtained with the bilinear transform with
prewarping:

```
y(k) = (1-α)/2 · [x(k) + x(k-1)] + α · y(k-1),   α = (1 - sin(ω0·T)) / cos(ω0·T)
```

## Authors

Luis Ricardo Rivera-Goitia  
Luis Enrique González-Jiménez  

Department of Electronics, Systems and Informatics  
ITESO – Universidad Jesuita de Guadalajara
