/*
==============================================================
Pedagogical DC Motor Control Platform - ESP32 Firmware V1.1
Authors: Luis Ricardo Rivera-Goitia, Luis Enrique González-Jiménez
ITESO – Universidad Jesuita de Guadalajara
Requires: esp32 by Espressif Systems 3.x (tested with 3.1.3)
--------------------------------------------------------------
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <PubSubClient.h>
#include "Adafruit_INA219.h"
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "esp_timer.h"
#include <Wire.h>

// INA219 configuration (register 0x00), same as the Adafruit library's
// (32 V, PGA /8 = 320 mV, 12-bit bus, continuous) but with averaged
// current measurement: 12 bits x 8 samples = 4.26 ms.
//   0x2000 BRNG 32V | 0x1800 PGA /8 | 0x0180 BADC 12b | 0x0058 SADC 8x | 0x0007 continuous
#define INA219_ADDR        0x40
#define INA219_CONFIG_AVG8 0x39DF

// ==========================================================
// Serial port debugging: 1 = enabled, 0 = disabled
// ==========================================================
#define DEBUG 0
#if DEBUG
  #define DBG(x)   Serial.println(x)
#else
  #define DBG(x)
#endif

// ==========================================================
// USER SETTINGS: replace with your own data before uploading
//   ssid / password        : WiFi network the ESP32 joins in MQTT mode
//                            (it must be a 2.4 GHz network)
//   mqttServer             : MQTT broker (HiveMQ Cloud cluster URL)
//   mqttUserName / mqttPwd : broker credentials (same as in the MATLAB GUI)
// ==========================================================
const char* ssid         = "YOUR_WIFI_SSID";
const char* password     = "YOUR_WIFI_PASSWORD";

const char* mqttServer   = "YOUR_CLUSTER_ID.s1.eu.hivemq.cloud";
const char* mqttUserName = "YOUR_MQTT_USERNAME";
const char* mqttPwd      = "YOUR_MQTT_PASSWORD";
const int   mqttPort     = 8883;

// Topics
const char* topic  = "ESP32/data";
const char* topic2 = "ESP32/signals";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// TCP Server
WiFiServer tcpServer(80);
WiFiClient tcpClient;

// TFT Screen
TFT_eSPI tft = TFT_eSPI();
int currentScreen = 0;
bool isTCPClientConnected = false;

// Current Sensor
Adafruit_INA219 ina219;

// Certificate (ISRG Root X1)
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

const float pi = 3.14159265358979323846;

// ==========================================================
// Pins
// ==========================================================
const int PWMA = 13;            // Pulse Width Modulation A
const int AIN2 = 12, AIN1 = 14; // motor inputs
const int STBY = 27;            // Activate the controller

#define ENC_A 34
#define ENC_B 35
#define ENC_PPR 11              // pulses per motor revolution (rising edge, phase A)
#define GEAR_BOX 44.4f          // measured (datasheet: 49)
#define ENC_TIMEOUT_US 50000UL  // no pulses in 50 ms -> speed = 0
#define ENC_MIN_PULSE_US 100UL  // pulses less than 100 us after the previous one are ignored
                                //     (at 12 V the real interval is ~880 us)

// ==========================================================
// Encoder (variables shared with the ISR)
// ==========================================================
volatile unsigned long t = 0;
volatile unsigned long t_m = 0;
volatile int signo = 1;
volatile int pos_i = 0;
volatile uint32_t pulseCount = 0;   // total rising edges received
portMUX_TYPE encMux = portMUX_INITIALIZER_UNLOCKED;

#define PWM_FREQ_HZ 20000           // motor PWM frequency

// ==========================================================
// Experiment parameters
// The MQTT/TCP callback writes them into "sharedP" and the control
// task copies them into "P" at the start of its next cycle.
// ==========================================================
enum Mode { MODE_NONE = 0, MODE_STEP, MODE_SINE, MODE_PID };

struct Params {
  bool  on;
  Mode  mode;
  float VCC, A, t0, offset;          // Step
  float omega, phi;                  // Sine (offset and A shared)
  bool  Continuous, Discrete;
  float T, N;
  double Kp, Ki, Kd;
  float des_speed;
  float omega0, omega0c;
  bool  filt, filtc;
};

Params sharedP = {};          // written by the callback
Params P = {};                // used by the control task
volatile uint32_t cmdSeq = 0; // incremented with each new command
portMUX_TYPE paramMux = portMUX_INITIALIZER_UNLOCKED;

// ==========================================================
// Sample queue (control task -> loop)
// ==========================================================
struct Sample {
  float  current;       // mA
  float  w;             // rad/s
  double time_s;        // real time since start
  float  Vt;            // V
  float  w_filt;
  float  current_filt;
  int    duty;          // %
  float  e;
};

// State of a low-pass filter (defined here, before any function, so that
// the Arduino IDE can generate the prototype of lowPass())
struct LowPass { float xk1 = 0, yk1 = 0; };

QueueHandle_t sampleQueue;
#define SAMPLE_QUEUE_LEN 400        // 2 s of samples at 5 ms
#define SEND_INTERVAL_MS 50         // one message every 50 ms
#define MAX_SAMPLES_PER_MSG 20      // 20 lines x ~55 B = ~1.1 kB
#define MQTT_BUFFER_SIZE 2048

// ==========================================================
// Control task and timer
// ==========================================================
TaskHandle_t controlTaskHandle = NULL;
esp_timer_handle_t ctrlTimer = NULL;
uint32_t ctrlPeriod_us = 5000;

// ==========================================================
// Prototypes
// ==========================================================
void showMainMenu();
void handleTouch(int x, int y);
void showMqttMenu();
void showWiFiMenu();
void startMQTTBroker();
void startHotspot();
bool setup_wifi();
void reconnect();
void callback(char* topic, byte* payload, unsigned int length);
void parseCommand(const char* json, size_t len);
bool jsonBool(JsonVariantConst v, bool def);
void IRAM_ATTR readEncoder();
float computeSpeed();
void resetSpeedEstimator();
void controlTask(void* pv);
void onControlTimer(void* arg);
void sendPendingSamples();

// Writes the INA219 configuration register
void ina219SetAveraging() {
  Wire.beginTransmission(INA219_ADDR);
  Wire.write(0x00);                               // configuration register
  Wire.write((uint8_t)(INA219_CONFIG_AVG8 >> 8));
  Wire.write((uint8_t)(INA219_CONFIG_AVG8 & 0xFF));
  Wire.endTransmission();
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
#if DEBUG
  Serial.begin(921600);   // with DEBUG all samples are sent: higher speed is needed
#else
  Serial.begin(115200);
#endif

  tft.init();
  tft.setRotation(3);
  showMainMenu();

  ina219.begin();
  ina219SetAveraging();       // average of 8 conversions
  ina219.getCurrent_mA();

  pinMode(ENC_A, INPUT);   // GPIO34/35 have no internal pull-up
  pinMode(ENC_B, INPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  analogWriteFrequency(PWMA, PWM_FREQ_HZ);   // 20 kHz (esp32 3.x API)
  analogWrite(PWMA, 0);

  attachInterrupt(digitalPinToInterrupt(ENC_A), readEncoder, RISING);

  // Sample queue
  sampleQueue = xQueueCreate(SAMPLE_QUEUE_LEN, sizeof(Sample));

  // Control task on core 1, higher priority than loop()
  xTaskCreatePinnedToCore(controlTask, "control", 8192, NULL, 3, &controlTaskHandle, 1);

  // Hardware timer that wakes up the control task
  esp_timer_create_args_t args = {};
  args.callback = &onControlTimer;
  args.name = "ctrl";
  esp_timer_create(&args, &ctrlTimer);
  esp_timer_start_periodic(ctrlTimer, ctrlPeriod_us);
}

// ==========================================================
// LOOP: screen, communications and data sending
// (the control NO longer runs here)
// ==========================================================
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
      if (!tcpClient || !tcpClient.connected()) {
        tcpClient = tcpServer.available();
        if (tcpClient) {
          DBG("TCP Client connected");
          isTCPClientConnected = true;
        }
      }

      if (tcpClient && tcpClient.connected()) {
        while (tcpClient.available()) {
          String msg = tcpClient.readStringUntil('\n');
          msg.trim();

          if (msg == "hello") {
            tcpClient.println("ESP32 ready");
            tcpClient.flush();
          } else {
            StaticJsonDocument<1024> jsonDoc;
            DeserializationError error = deserializeJson(jsonDoc, msg);
            if (!error) {
              parseCommand(msg.c_str(), msg.length());
              tcpClient.println("OK");
              tcpClient.flush();
            } else {
              tcpClient.println("Unknown command.");
              tcpClient.flush();
            }
          }
        }
      }
    }
  }

  // Send the accumulated samples
  sendPendingSamples();

  delay(1);   // yields the processor to other tasks
}

// ==========================================================
// Sending samples in batches
// ==========================================================
void sendPendingSamples() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend < SEND_INTERVAL_MS) return;
  lastSend = millis();

  bool mqttOK = (currentScreen == 3 && client.connected());
  bool tcpOK  = (tcpClient && tcpClient.connected());

  Sample s;
  while (uxQueueMessagesWaiting(sampleQueue) > 0) {
    String batch;
    batch.reserve(MAX_SAMPLES_PER_MSG * 64);
    int n = 0;

    while (n < MAX_SAMPLES_PER_MSG && xQueueReceive(sampleQueue, &s, 0) == pdTRUE) {
      batch += String(s.current) + "," + String(s.w) + "," + String(s.time_s, 6) + "," +
               String(s.Vt) + "," + String(s.w_filt) + "," + String(s.current_filt) + "," +
               String(s.duty) + "," + String(s.e) + "\n";
      n++;
    }
    if (n == 0) break;

    if (mqttOK) client.publish(topic2, batch.c_str());
    if (tcpOK)  tcpClient.print(batch);
#if DEBUG
    Serial.print(batch);
#endif
  }
}

// ==========================================================
// Communications: WiFi / MQTT / TCP / screen
// ==========================================================
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
  client.setBufferSize(MQTT_BUFFER_SIZE);   // several samples per message
  client.setCallback(callback);
  reconnect();
}

void startHotspot() {
  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP("ESP32_Hotspot", "12345678");
  if (!success) {
    DBG("SoftAP failed to start!");
    return;
  }

  IPAddress IP = WiFi.softAPIP();
  tcpServer.begin();

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

// ==========================================================
// JSON commands (MQTT and TCP use the same function)
// ==========================================================
void callback(char* topic, byte* payload, unsigned int length) {
  parseCommand((const char*)payload, length);
}

// Accepts true/false or 1/0 (MATLAB may send either)
bool jsonBool(JsonVariantConst v, bool def) {
  if (v.isNull())     return def;
  if (v.is<bool>())   return v.as<bool>();
  if (v.is<float>())  return v.as<float>() != 0.0f;
  return def;
}

void parseCommand(const char* json, size_t len) {
  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, json, len)) return;

  const char* interruptor = doc["interruptor"];
  if (interruptor == nullptr) return;

  Params np;
  portENTER_CRITICAL(&paramMux);
  np = sharedP;                       // starts from the current values
  portEXIT_CRITICAL(&paramMux);

  if (strcmp(interruptor, "off") == 0) {
    np.on = false;
  } else if (strcmp(interruptor, "on") == 0) {
    np.on = true;

    const char* typeChar = doc["Type"];
    if (typeChar != nullptr) {
      if      (strcmp(typeChar, "Step") == 0) np.mode = MODE_STEP;
      else if (strcmp(typeChar, "Sine") == 0) np.mode = MODE_SINE;
      else if (strcmp(typeChar, "PID")  == 0) np.mode = MODE_PID;
    }

    np.VCC        = doc["VCC"]        | 0.0f;
    np.A          = doc["A"]          | 0.0f;
    np.t0         = doc["t0"]         | 0.0f;
    np.offset     = doc["offset"]     | 0.0f;
    np.omega      = doc["omega"]      | 0.0f;
    np.phi        = doc["phi"]        | 0.0f;
    np.Continuous = jsonBool(doc["Continuous"], true);
    np.Discrete   = jsonBool(doc["Discrete"],   false);
    np.T          = doc["T"]          | 0.0f;
    np.N          = doc["N"]          | 0.0f;
    np.Kp         = doc["Kp"]         | 0.0;
    np.Ki         = doc["Ki"]         | 0.0;
    np.Kd         = doc["Kd"]         | 0.0;
    np.des_speed  = doc["des_speed"]  | 0.0f;
    np.omega0     = doc["omega0"]     | 0.0f;
    np.omega0c    = doc["omega0c"]    | 0.0f;
    np.filt       = jsonBool(doc["filt"],  false);
    np.filtc      = jsonBool(doc["filtc"], false);
  } else {
    return;
  }

  portENTER_CRITICAL(&paramMux);
  sharedP = np;
  cmdSeq++;
  portEXIT_CRITICAL(&paramMux);
}

// ==========================================================
// Encoder
// ==========================================================
void IRAM_ATTR readEncoder() {
  unsigned long now = micros();
  int val = digitalRead(ENC_B);

  portENTER_CRITICAL_ISR(&encMux);
  if (t != 0 && (now - t) < ENC_MIN_PULSE_US) {   // false pulse (noise): ignored
    portEXIT_CRITICAL_ISR(&encMux);
    return;
  }
  t_m = t;
  t = now;
  signo = (val == LOW) ? 1 : -1;
  pos_i += (val > 0) ? -1 : 1;
  pulseCount++;
  portEXIT_CRITICAL_ISR(&encMux);
}

// Speed estimator state (used only by the control task)
uint32_t      spdPrevCount = 0;     // accumulated pulses at the previous sample
unsigned long spdPrevLast  = 0;     // time of the last pulse at the previous sample
bool          spdValid     = false;
int           spdPrevPos   = 0;     // position (signed pulses) at the previous sample
int           spdDir       = 1;     // last confirmed direction of rotation

void resetSpeedEstimator() {
  portENTER_CRITICAL(&encMux);
  spdPrevCount = pulseCount;
  spdPrevLast  = t;
  spdPrevPos   = pos_i;
  spdDir       = signo;
  portEXIT_CRITICAL(&encMux);
  spdValid = false;
}

// Wheel speed (rad/s), averaged over all pulses received
// since the previous sample. 0 if the motor is stopped.
float computeSpeed() {
  static bool spdFlipPending = false;   // direction change pending confirmation
  unsigned long t_now, t_prev;
  uint32_t count;
  int s, pos;

  portENTER_CRITICAL(&encMux);
  t_now = t;
  t_prev = t_m;
  count = pulseCount;
  s = signo;
  pos = pos_i;
  portEXIT_CRITICAL(&encMux);

  // No recent pulses: stopped
  if (t_now == 0 || (micros() - t_now) > ENC_TIMEOUT_US) {
    spdPrevCount = count;
    spdPrevLast = t_now;
    spdPrevPos = pos;
    spdDir = s;
    spdFlipPending = false;
    spdValid = false;
    return 0.0;
  }

  uint32_t dN = count - spdPrevCount;

  // Direction of rotation from the net pulse balance over the period (not from
  // a single pulse). A direction change is accepted if indicated by 2 or more net
  // pulses, or if it repeats in two consecutive periods.
  if (dN > 0) {
    int dPos = pos - spdPrevPos;
    int newDir = (dPos > 0) ? 1 : ((dPos < 0) ? -1 : spdDir);
    if (newDir != spdDir) {
      if (abs(dPos) >= 2 || spdFlipPending) {
        spdDir = newDir;
        spdFlipPending = false;
      } else {
        spdFlipPending = true;
      }
    } else {
      spdFlipPending = false;
    }
    spdPrevPos = pos;
  }
  float period_us = 0.0f;

  if (dN > 0 && spdValid) {
    // dN complete intervals between the previous last pulse and the current one
    period_us = (float)(t_now - spdPrevLast) / (float)dN;
  } else if (spdValid && t_prev != 0 && t_now != t_prev && (t_now - t_prev) <= ENC_TIMEOUT_US) {
    // No new pulse (low speed) or first sample: last interval
    period_us = (float)(t_now - t_prev);
  }

  if (dN > 0) {
    spdPrevCount = count;
    spdPrevLast = t_now;
    spdValid = true;
  }

  if (period_us <= 0.0f) return 0.0;
  return (2.0f * pi * 1.0e6f * spdDir) / (period_us * ENC_PPR * GEAR_BOX);
}

// ==========================================================
// Motor
// ==========================================================
void setMotor(int duty, int pwmValue) {
  if (duty > 0) {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, pwmValue);
  } else if (duty < 0) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    analogWrite(PWMA, pwmValue);
  } else {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 0);
  }
}

// Converts a voltage into a duty cycle and applies it. Returns the duty (%).
int applyVoltage(float V, float VCC) {
  int duty = 0;
  if (VCC > 0.0f) duty = (int)((V / VCC) * 100.0f);   // no division by zero
  duty = constrain(duty, -100, 100);                  // limited to +/-100 %
  int pwmValue = abs(map(duty, -100, 100, -255, 255));
  setMotor(duty, pwmValue);
  return duty;
}

// ==========================================================
// First-order low-pass filter (bilinear transform with prewarping)
// ==========================================================
float lowPass(LowPass& f, float x, float Ts, float f0_hz) {
  float w0 = f0_hz * 2.0f * pi;
  float denom = cos(Ts * w0);
  if (fabs(denom) < 0.0001f) denom = 0.0001f;
  float alpha = constrain((1.0f - sin(Ts * w0)) / denom, 0.0f, 0.99f);
  float a0 = (1.0f - alpha) / 2.0f;
  float y = a0 * (x + f.xk1) + alpha * f.yk1;
  f.xk1 = x;
  f.yk1 = y;
  return y;
}

// ==========================================================
// Timer -> wakes up the control task
// ==========================================================
void onControlTimer(void* arg) {
  if (controlTaskHandle) xTaskNotifyGive(controlTaskHandle);
}

void setControlPeriod(uint32_t period_us) {
  if (period_us == ctrlPeriod_us) return;
  ctrlPeriod_us = period_us;
  esp_timer_stop(ctrlTimer);
  esp_timer_start_periodic(ctrlTimer, ctrlPeriod_us);
}

// ==========================================================
// CONTROL TASK
// ==========================================================
void controlTask(void* pv) {
  uint32_t appliedSeq = 0;
  bool running = false;
  Mode runMode = MODE_NONE;

  unsigned long t_start_us = 0, prev_us = 0;
  float integral = 0, prev_e = 0, prev_derivative = 0;
  bool  pidFirst = true;                 // first PID sample after a start
  LowPass fSpeed, fCurrent;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // waits for the next period

    // ---------- New command ----------
    uint32_t seq;
    portENTER_CRITICAL(&paramMux);
    seq = cmdSeq;
    if (seq != appliedSeq) P = sharedP;
    portEXIT_CRITICAL(&paramMux);

    if (seq != appliedSeq) {
      appliedSeq = seq;

      if (!P.on) {                               // ----- OFF -----
        running = false;
        runMode = MODE_NONE;
        applyVoltage(0.0f, 1.0f);
        setControlPeriod(5000);
        continue;
      }

      // Period: discrete (Sine/PID) uses T; everything else, 5 ms
      bool discreto = (P.Discrete && (P.mode == MODE_SINE || P.mode == MODE_PID));
      float Tsec = (P.T < 0.005f) ? 0.005f : P.T;
      setControlPeriod(discreto ? (uint32_t)(Tsec * 1.0e6f) : 5000);

      // Same mode and already running: only update parameters
      if (!(running && P.mode == runMode)) {
        running = true;
        runMode = P.mode;
        t_start_us = micros();
        resetSpeedEstimator();
        prev_us = t_start_us - ctrlPeriod_us;  // first dt = nominal period (not ~0)
        integral = prev_e = prev_derivative = 0;
        pidFirst = true;
        fSpeed = LowPass();
        fCurrent = LowPass();
        xQueueReset(sampleQueue);
      }
    }

    if (!running) continue;

    // ---------- Time ----------
    unsigned long now = micros();
    float Ts_nom = ctrlPeriod_us / 1.0e6f;
    float dt = (now - prev_us) / 1.0e6f;
    prev_us = now;
    if (dt <= 0.0f || dt > 2.0f * Ts_nom) dt = Ts_nom;
    double t_rel = (now - t_start_us) / 1.0e6;

    // Time step for filters and PID: real (continuous) or T (discrete)
    float Ts = P.Discrete ? Ts_nom : dt;

    // ---------- Measurements ----------
    float current = ina219.getCurrent_mA();
    float w = computeSpeed();
    float current_f = P.filtc ? lowPass(fCurrent, current, Ts, P.omega0c) : 0.0f;
    float w_f       = P.filt  ? lowPass(fSpeed,   w,       Ts, P.omega0)  : 0.0f;

    float Vt = 0.0f, e = 0.0f;
    float Vreport;     // value sent to the GUI

    // ---------- Control law ----------
    if (runMode == MODE_STEP) {
      Vt = (t_rel < P.t0) ? P.offset : P.A + P.offset;

    } else if (runMode == MODE_SINE) {
      double tk = t_rel;
      if (P.Discrete && P.T > 0) tk = floor(t_rel / P.T) * P.T;
      Vt = P.A * sin(P.omega * tk + P.phi) + P.offset;

    } else if (runMode == MODE_PID) {
      float w_used = P.filt ? w_f : w;
      e = P.des_speed - w_used;
      if (pidFirst) {                       // e(k-1) does not exist yet:
        prev_e = e;                         // no derivative kick at start
        pidFirst = false;
      }

      float proportional = P.Kp * e;

      float derivative;
      if (P.Discrete && P.N > 0) {          // derivative filter with coefficient N
        derivative = P.Kd * P.N * (e - prev_e) + prev_derivative * (1.0f - Ts * P.N);
      } else {
        derivative = (P.Kd / Ts) * (e - prev_e);
      }

      float integral_candidate = integral + e * Ts * P.Ki;
      float Vk_unsat = proportional + integral_candidate + derivative;

      // Basic anti-windup (conditional integration)
      bool allowIntegration = (Vk_unsat <= 12.0f && Vk_unsat >= -12.0f) ||
                              (Vk_unsat > 12.0f && e < 0) ||
                              (Vk_unsat < -12.0f && e > 0);
      if (allowIntegration) integral = integral_candidate;

      float u = proportional + integral + derivative;   // unsaturated PID output
      Vt = constrain(u, -12.0f, 12.0f);                 // voltage applied to the motor
      Vreport = u;

      prev_e = e;
      prev_derivative = derivative;

    }

    if (runMode != MODE_PID) Vreport = Vt;              // Step/Sine: applied voltage
    int duty = applyVoltage(Vt, P.VCC);

    // ---------- Store sample ----------
    Sample s;
    s.current = current;
    s.w = w;
    s.time_s = t_rel;
    s.Vt = Vreport;
    s.w_filt = w_f;
    s.current_filt = current_f;
    s.duty = duty;
    s.e = e;
    xQueueSend(sampleQueue, &s, 0);   // if the queue is full, it is discarded
  }
}