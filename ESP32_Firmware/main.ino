/*
==============================================================
Pedagogical DC Motor Control Platform
ESP32 Firmware

Authors:
Luis Ricardo Rivera-Goitia
Luis Enrique González-Jiménez

Institution:
Department of Electronics, Systems and Informatics
ITESO – Universidad Jesuita de Guadalajara

Description:
Firmware for the IoT-based DC motor control platform used
for teaching automatic control and embedded systems.

==============================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <PubSubClient.h>
#include "Adafruit_INA219.h"
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// ==========================================================
// WiFi Configuration
// Replace with your own WiFi credentials
// ==========================================================

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ==========================================================
// MQTT Broker Configuration
// Replace with your own MQTT broker credentials
// ==========================================================

const char* mqttServer = "58b62bdfd8164226ab2138351ad11e6b.s1.eu.hivemq.cloud";
const char* mqttUserName = "YOUR_MQTT_USERNAME";
const char* mqttPwd = "YOUR_MQTT_PASSWORD";
const int mqttPort = 8883;

// Topics
const char* topic = "ESP32/data";
const char* topic2 = "ESP32/signals";


WiFiClientSecure espClient;
PubSubClient client(espClient);

// TCP Server
WiFiServer tcpServer(80);
WiFiClient tcpClient;
// TFT Screen declaration & state
TFT_eSPI tft = TFT_eSPI();
int currentScreen = 0;

bool isTCPClientConnected = false;
//String tcpBuffer = "";

// Current Sensor
Adafruit_INA219 ina219;

// Certificate
static const char *root_ca PROGMEM = R"EOF(
  -----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Global variables
float prev_Ki, e, e_filt, prev_e, prev_e_filt, N, filt, filtc, deltaT=0.0, T=0.0, timediscrete;
float velocity, v, velt, des_speed, u, proportional, proportional_filt, integral, integral_filt, derivative_filt, derivative, prev_derivative, prev_derivative_filt;
double Kp = 0.0, Ki = 0.0, Kd = 0.0;
float w_filt, alpha, a0, xk1 = 0, yk1 = 0, xk1c = 0, yk1c = 0, omega0, omega0c, omega0_rad,omega0c_rad;
const float pi = 3.14159265358979323846;
long prevT;
int posPrev, pos;
volatile int pos_i = 0;
const char* interruptor;
bool Continuous=true, Discrete=false;
String Type = "", msgStr = "";
float VCC = 0.0, A = 0.0, current_mA = 0, current_mA2= 0, w = 0.0, wk, Vt = 0.0,Vt_filt = 0.0, Vk = 0.0, Vk_filt = 0.0;
float motorStatus = 0, omega, phi, t0, new_t0 = 0, offset = 0, new_offset = 0;
int signo = 1, dutyCycle = 0, dutyCycle_filt = 0,dutyCycleMapped, pwmValue = 0, pwmValue_filt = 0, counter = 0;
unsigned long previousMillis = 0, currentMillis = 0, tiempoBase = 0, tiempoActual = 0;
const long interval = 50;
long PIDinterval = interval;
long currT;

// Counter for milliseconds during interval
volatile unsigned long t = 0;
volatile unsigned long t_m = 0;

// Motor driver pin definition
const int PWMA = 13;  // Pulse Width Modulation A
const int AIN2 = 12, AIN1 = 14; // motor inputs
const int STBY = 27;  // Activate the controller

// Encoder pin definition
#define ENC_A 34
#define ENC_B 35
// DC Motor 130RPM
#define ENC_PPR 11
#define GEAR_BOX 49

// ========================== FUNCTION DECLARATIONS =========================

void showMainMenu();
void handleTouch(int x, int y);
void showMqttMenu();
void showWiFiMenu();
void startMQTTBroker();
void startHotspot();
void actualizarTiempoBase();
void resetControlVariables();
void callback(char* topic, byte* payload, unsigned int length);
void sendDataToClient();
void readEncoder();
void controlMotor(int motorStatus, int dutyCycle, int pwmValue);
void StepControl();
float ContinuousSineResponse(float A, float omega, float phi, float tiempoActual, float offset);
float DiscreteSineResponse(float A, float omega, float phi, float tiempoActual, float offset, float T);
void SineControl();
void PIDControl();
void handleJsonCommand(String data);
bool setup_wifi();
void reconnect();

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(3);
  showMainMenu();
  //delay(300);
  ina219.begin();
  ina219.getCurrent_mA();
  // Motor pins setup
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);

  attachInterrupt(digitalPinToInterrupt(ENC_A), readEncoder, RISING);

}

void loop() {
  // Touch screen input
  uint16_t raw_x, raw_y;
  if (tft.getTouch(&raw_x, &raw_y)) {
    uint16_t x = map(raw_y, 240, 0, 0, 320);
    uint16_t y = map(raw_x, 320, 0, 0, 240);
    handleTouch(x, y);
    delay(200);
  }

  // MQTT Mode
  if (currentScreen == 3 && client.connected()) {
    client.loop();
  }

  // TCP Mode
  if (currentScreen == 4) {
    if (WiFi.getMode() == WIFI_AP || WiFi.status() == WL_CONNECTED) {
      // Accept TCP client if not yet connected
      if (!tcpClient || !tcpClient.connected()) {
        tcpClient = tcpServer.available();
        if (tcpClient) {
          Serial.println("TCP Client connected");
          isTCPClientConnected = true;
        }
      }

      // If a TCP client is connected and sending data
      if (tcpClient && tcpClient.connected()) {
        while (tcpClient.available()) {
          String msg = tcpClient.readStringUntil('\n');
          msg.trim(); // Remove leading/trailing whitespace
          Serial.print("Received: ");
          Serial.println(msg);

          if (msg == "hello") {
            tcpClient.println("ESP32 ready");
            tcpClient.flush();
            Serial.println("Sent: ESP32 ready.");
          } else {
            // Try parsing JSON
            StaticJsonDocument<1024> jsonDoc;
            DeserializationError error = deserializeJson(jsonDoc, msg);

            if (!error) {
              handleJsonCommand(msg);
              tcpClient.println("OK");
              tcpClient.flush();
              Serial.println("Parsed JSON and updated control variables.");
            } else {
              tcpClient.println("Unknown command.");
              tcpClient.flush();
              Serial.println("Invalid JSON received.");
            }
          }
        }
      }
    }
  }

  // Time update
  currentMillis = millis();
  tiempoActual = currentMillis - tiempoBase;

  // Control selection
  if (Type == "Step") StepControl();
  else if (Type == "Sine") SineControl();
  else if (Type == "PID") PIDControl();
}

bool setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) delay(500);
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.disconnect(true);
  return false;
}

void reconnect() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 40);
  tft.println("MQTT Broker Mode");
  tft.setCursor(20, 80);
  tft.setTextColor(TFT_WHITE);
  tft.println("Status: Connecting...");
  int retryCount = 0;
  while (!client.connected() && retryCount++ < 5) {
    if (client.connect("ESP32Client", mqttUserName, mqttPwd)) {
      client.subscribe("ESP32/data");
      client.subscribe("ESP32/signals");
      tft.fillRect(0, 80, 320, 120, TFT_BLACK);
      tft.setCursor(20, 80);
      tft.setTextColor(TFT_GREEN);
      tft.println("Status: Connected!");
      tft.setCursor(20, 120);
      tft.setTextColor(TFT_WHITE);
      tft.print("Topics Subscribed");
      tft.fillRect(30, 190, 260, 40, TFT_RED);
      tft.setCursor(50, 200);
      tft.println("Go back to Main Menu");
      currentScreen = 3;
      return;
    }
    delay(500);
  }
  tft.setTextColor(TFT_RED);
  tft.setCursor(20, 150);
  tft.println("MQTT Connection Failed.");
  tft.fillRect(30, 190, 260, 40, TFT_RED);
  tft.setCursor(60, 205);
  tft.setTextColor(TFT_WHITE);
  tft.println("Disconnect MQTT");
  currentScreen = 5;
}

void showMainMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("Motor Control Interface");

  tft.fillRect(60, 50, 210, 60, TFT_BLUE);
  tft.setCursor(75, 72);
  tft.println("1: MQTT Broker");

  tft.fillRect(60, 130, 210, 60, TFT_RED);
  tft.setCursor(75, 150);
  tft.println("2: WiFi Computer");
  currentScreen = 0;
}

void handleTouch(int x, int y) {
  if (currentScreen == 0) {
    if (x > 60 && x < 260 && y > 50 && y < 110) showMqttMenu();
    else if (x > 60 && x < 260 && y > 130 && y < 190) showWiFiMenu();
  } else if ((currentScreen == 1 || currentScreen == 2) && x > 30 && x < 250) {
    if (y > 60 && y < 110) (currentScreen == 1) ? startMQTTBroker() : startHotspot();
    else if (y > 140 && y < 190) showMainMenu();
  } else if ((currentScreen == 3 || currentScreen == 4 || currentScreen == 5) && x > 30 && x < 290 && y > 190 && y < 230) {
    client.disconnect();
    WiFi.disconnect(true);
    showMainMenu();
  }
}

void showMqttMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("Select Connection Method");
  tft.fillRect(30, 60, 220, 50, TFT_BLUE);
  tft.setCursor(40, 80);
  tft.println("Connect to MQTT");
  tft.fillRect(30, 140, 220, 50, TFT_GREEN);
  tft.setCursor(40, 155);
  tft.println("Back to Main Menu");
  currentScreen = 1;
}

void showWiFiMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("Select Connection Method");
  tft.fillRect(30, 60, 220, 50, TFT_BLUE);
  tft.setCursor(40, 80);
  tft.println("Connect to WiFi");
  tft.fillRect(30, 140, 220, 50, TFT_GREEN);
  tft.setCursor(40, 155);
  tft.println("Back to Main Menu");
  currentScreen = 2;
}

void startMQTTBroker() {
  // Show connecting message first
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.println("Connecting...");
  tft.setCursor(20, 100);
  tft.setTextColor(TFT_YELLOW);
  tft.println("Please wait...");

  delay(500);

  if (!setup_wifi()) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(20, 60);
    tft.println("Status: Connection Failed");
    tft.setCursor(20, 100);
    tft.setTextColor(TFT_YELLOW);
    tft.println("Is Hotspot On?");
    tft.fillRect(30, 190, 260, 40, TFT_RED);
    tft.setCursor(50, 200);
    tft.setTextColor(TFT_WHITE);
    tft.println("Go back to Main Menu");
    currentScreen = 5;
    return;
  }
  espClient.setCACert(root_ca);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  reconnect();
}

void startHotspot() {
  WiFi.mode(WIFI_AP); // Force AP mode
  bool success = WiFi.softAP("ESP32_Hotspot", "12345678");

  if (success) {
    Serial.println("SoftAP started successfully");
  } else {
    Serial.println("SoftAP failed to start!");
    return; // Abort if SoftAP failed
  }

  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP32 Hotspot IP: ");
  Serial.println(IP);

  tcpServer.begin();

  // Now update your TFT screen here
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 40);
  tft.println("SoftAP Mode Active");
  tft.setCursor(20, 80);
  tft.print("IP: ");
  tft.println(IP);

  tft.fillRect(30, 190, 260, 40, TFT_RED);
  tft.setCursor(50, 200);
  tft.setTextColor(TFT_WHITE);
  tft.println("Go back to Main Menu");

  currentScreen = 4;
}

void actualizarTiempoBase() {
  // Esta función se llama cuando se necesita restablecer el tiempo base a cero
  tiempoBase = millis();
  currentMillis = 0;
}

void resetControlVariables() {
  T = e = prev_e = Vt = N = alpha = a0 = VCC = A = omega = omega0 = omega0c = phi = 0;
  proportional = integral = derivative = Ki = Kd = w_filt = w = xk1 = yk1 = xk1c = yk1c = current_mA = current_mA2 = 0.0;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String data = "";
  /*for (int i = 0; i < length; i++) {
    data += (char)payload[i];
  }*/
  for (int i = 0; i < length; i++) data += (char)payload[i];
  //handleJsonCommand(data);
  StaticJsonDocument<1024> jsonDoc;
  if (deserializeJson(jsonDoc, data)) return;

  // Extract and store individual fields
  interruptor = jsonDoc["interruptor"];
  const char* typeChar = jsonDoc["Type"]; // Step, Sine, PID
  Type = typeChar;
  T = jsonDoc["T"].as<float>();
  N = jsonDoc["N"];
  VCC = jsonDoc["VCC"];
  A = jsonDoc["A"]; // Amplitude
  omega = jsonDoc["omega"];
  omega0 = jsonDoc["omega0"];
  omega0c = jsonDoc["omega0c"];
  phi = jsonDoc["phi"];
  Continuous = jsonDoc["Continuous"];
  Discrete = jsonDoc["Discrete"];
  filt = jsonDoc["filt"];
  filtc = jsonDoc["filtc"];
  Kp = jsonDoc["Kp"];
  Ki = jsonDoc["Ki"];
  Kd = jsonDoc["Kd"];
  des_speed = jsonDoc["des_speed"];
  new_t0 = jsonDoc["t0"]; // stores de new value of t0
  new_offset = jsonDoc["offset"]; // stores the new value of offset

  if (strcmp(interruptor, "on") == 0) {    
    motorStatus = 1;
    digitalWrite(AIN1, HIGH);
    currentMillis = 0;
    if (new_t0 != t0 || new_offset != offset) {
      t0 = new_t0; // Updates t0 with the new value received
      offset = new_offset; // Updates offset with the new value received
      actualizarTiempoBase(); // Call the function to reset the base time
    } else if (new_t0 == t0) { // if t0 hasn't changed when cliking start button
      t0 = new_t0; // 
      actualizarTiempoBase(); // Call the function to reset the base time
    }
  }

  if (strcmp(interruptor, "off") == 0) {
        motorStatus = 0;
        dutyCycle = 0;
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, LOW);
        resetControlVariables();
  }
}

void sendDataToClient() {
  
  String msgStr = String(current_mA) + "," + String(w) + "," + String(currentMillis / 1000.0) + "," + String(Vt) + "," + String(w_filt) + "," + String(current_mA2) + "," + String(dutyCycle) + "," + String(e) + "\n";
  Serial.println(msgStr);
  client.publish(topic2, msgStr.c_str());
  if (isTCPClientConnected && tcpClient.connected()) {
    Serial.println("TCP client is connected, sending message...");
    Serial.println(msgStr);  // Print the message to the Serial Monitor
    tcpClient.print(msgStr); // Send the message via TCP
  } else {
    Serial.println("TCP client not connected.");
  }
}

void readEncoder() {
  t_m = t;
  t = micros();

  int val = digitalRead(ENC_B); // Reading ENC_B

  signo = (val == LOW) ? 1 : -1;   // Forward or reverse direction
  int increment = (val > 0) ? -1 : 1; // Increment value based on direction

  pos_i += increment; // Update position counter
}

// Define the function to control the motor based on motor status and duty cycle
void controlMotor(int motorStatus, int dutyCycle, int pwmValue) {
  if (motorStatus == 1) {
    if (dutyCycle > 0) {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      analogWrite(PWMA, pwmValue);
    } else if (dutyCycle < 0) {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
      analogWrite(PWMA, pwmValue);
    } else {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, LOW);
      analogWrite(PWMA, 0);
    }
  }
}


void StepControl() {
  
  if ((tiempoActual / 1000.0) < t0) {
      Vt = offset;
  } else {
      Vt = A + offset;
  }

  dutyCycle = (Vt / VCC) * 100;
    
  int dutyCycleMapped = map(dutyCycle, -100, 100, -255, 255);
  pwmValue = abs(dutyCycleMapped);
  Serial.println(dutyCycleMapped);
  controlMotor(motorStatus, dutyCycle, pwmValue);

  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      current_mA = 0;
      current_mA = ina219.getCurrent_mA();
      w = (2 * 3.141569 * 1.0e6 * signo) / ((t - t_m) * ENC_PPR*GEAR_BOX);
        
      sendDataToClient();
  }
}

// Function to calculate V(t) for the Continuous Sine Response
float ContinuousSineResponse(float A, float omega, float phi, float tiempoActual, float offset) {
  return A * sin((omega * tiempoActual/1000.0) + phi) + offset;
}

// Function to calculate V(t) for the Discrete Sine Response
float DiscreteSineResponse(float A, float omega, float phi, float tiempoActual, float offset, float T) {
    timediscrete = (tiempoActual / 1000.0)* T; // Compute the time within one period
    return A * sin((omega * timediscrete) + phi) + offset;
}

void SineControl() {
    
  Vt = ContinuousSineResponse(A, omega, phi, tiempoActual, offset);
  dutyCycle = (Vt / VCC) * 100;
  int dutyCycleMapped = map(dutyCycle, -100, 100, -255, 255);
  pwmValue = abs(dutyCycleMapped);
  Serial.println(dutyCycleMapped);

  if (Discrete == 1) {
    Vt = DiscreteSineResponse(A, omega, phi, tiempoActual, offset, T);
    dutyCycle = (Vt / VCC) * 100;
    dutyCycleMapped = map(dutyCycle, -100, 100, -255, 255);
    pwmValue = abs(dutyCycleMapped);
  }

  controlMotor(motorStatus, dutyCycle, pwmValue);

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    current_mA = 0;
    current_mA = ina219.getCurrent_mA();
    w = (2 * 3.141569 * 1.0e6 * signo) / ((t - t_m) * ENC_PPR*GEAR_BOX);
        
    sendDataToClient();
  }
}

void PIDControl() {
  // === Delta T ===
  currT = micros();
  deltaT = ((float)(currT - prevT)) / 1.0e6;
  prevT = currT;
  if (deltaT <= 0.0) deltaT = 0.005;

  // === Desired speed and error ===
  wk = des_speed;
  e = wk - w;
  proportional = Kp * e;

  // === PID Interval ===
  if (Continuous == 1) {
    PIDinterval = 5;
    integral += (prev_e * deltaT * Ki);
    derivative = (Kd / deltaT) * (e - prev_e);
  }

  if (Discrete == 1) {
    PIDinterval = T * 1000;
    integral += (prev_e * T * Ki);
    if (N == true) {
      derivative = Kd * N * (e - prev_e) + prev_derivative * (1 - T * N);
    } else {
      derivative = (Kd / T) * (e - prev_e);
    }
  }

  Vk = proportional + integral + derivative;

  // === Saturation ===
  if (Vk > 12.0) Vk = 12;
  else if (Vk < -12.0) Vk = -12;
  Vt = Vk;

  dutyCycle = (Vk / VCC) * 100;
  int dutyCycleMapped = map(dutyCycle, -100, 100, -255, 255);
  pwmValue = abs(dutyCycleMapped);

  controlMotor(motorStatus, dutyCycle, pwmValue);

  // === Store Previous ===
  prev_e = e;
  prev_Ki = Ki;
  prev_derivative = derivative;

  if (currentMillis - previousMillis >= PIDinterval) {
    previousMillis = currentMillis;

    // === Get New Current ===
    current_mA = ina219.getCurrent_mA();

    // === Filter Current (same method as speed filter) ===
    if (filtc) {
      omega0c_rad = omega0c * 2 * pi;
      float denom = (Continuous == 1) ? cos(deltaT * omega0c_rad) : cos(T * omega0c_rad);
      if (abs(denom) < 0.0001) denom = 0.0001;
      float raw_alpha = (1 - sin((Continuous == 1 ? deltaT : T) * omega0c_rad)) / denom;
      alpha = constrain(raw_alpha, 0.0, 0.99);
      a0 = (1 - alpha) / 2;

      current_mA2 = a0 * (current_mA + xk1c) + alpha * yk1c;
      xk1c = current_mA;
      yk1c = current_mA2;
    }

    // === Compute Angular Speed ===
    w = (2 * pi * 1.0e6 * signo) / ((t - t_m) * ENC_PPR * GEAR_BOX);

    // === Filter Speed ===
    if (filt) {
      omega0_rad = omega0 * 2 * pi;
      float denom = (Continuous == 1) ? cos(deltaT * omega0_rad) : cos(T * omega0_rad);
      if (abs(denom) < 0.0001) denom = 0.0001;
      float raw_alpha = (1 - sin((Continuous == 1 ? deltaT : T) * omega0_rad)) / denom;
      alpha = constrain(raw_alpha, 0.0, 0.99);
      a0 = (1 - alpha) / 2;

      w_filt = a0 * (w + xk1) + alpha * yk1;
      xk1 = w;
      yk1 = w_filt;
    }

    // === Send Data ===
    sendDataToClient();
  }
}

void handleJsonCommand(String data) {
  StaticJsonDocument<1024> jsonDoc;
  if (deserializeJson(jsonDoc, data)) return;

  // Extract fields
  interruptor = jsonDoc["interruptor"];
  const char* typeChar = jsonDoc["Type"]; // Step, Sine, PID
  if (typeChar != nullptr) Type = typeChar;

  T = jsonDoc["T"].as<float>();
  N = jsonDoc["N"];
  VCC = jsonDoc["VCC"];
  A = jsonDoc["A"];
  omega = jsonDoc["omega"];
  omega0 = jsonDoc["omega0"];
  omega0c = jsonDoc["omega0c"];
  phi = jsonDoc["phi"];
  Continuous = jsonDoc["Continuous"];
  Discrete = jsonDoc["Discrete"];
  filt = jsonDoc["filt"];
  filtc = jsonDoc["filtc"];
  Kp = jsonDoc["Kp"];
  Ki = jsonDoc["Ki"];
  Kd = jsonDoc["Kd"];
  des_speed = jsonDoc["des_speed"];
  new_t0 = jsonDoc["t0"];
  new_offset = jsonDoc["offset"];

  if (strcmp(interruptor, "on") == 0) {
    motorStatus = 1;
    digitalWrite(AIN1, HIGH);
    currentMillis = 0;
    if (new_t0 != t0 || new_offset != offset) {
      t0 = new_t0;
      offset = new_offset;
      actualizarTiempoBase();
    } else if (new_t0 == t0) {
      t0 = new_t0;
      actualizarTiempoBase();
    }
  }

  if (strcmp(interruptor, "off") == 0) {
    motorStatus = 0;
    dutyCycle = 0;
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    resetControlVariables();
  }
}
