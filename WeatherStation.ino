
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include "secrets.h"

void syncTime();
void ensureWiFi();
void networkTask(void *param);

// ===== WiFi =====
const char* WIFI_SSID = WIFI_SSID_VAL;
const char* WIFI_PASS = WIFI_PASS_VAL;

// ===== Firebase =====
const char* FIREBASE_HOST = FIREBASE_HOST_VAL;
const char* FIREBASE_API_KEY = FIREBASE_API_KEY_VAL;

// ===== Display (GC9A01 over SPI) =====
#define TFT_SCLK 4
#define TFT_MOSI 6
#define TFT_CS   7
#define TFT_DC   5
#define TFT_RST  10
#define TFT_BL   3
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /* rotation */, true /* IPS */);

// colours (RGB565)
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_BLUE   0x001F
#define C_YELLOW 0xFFE0
#define C_CYAN   0x07FF
#define C_ORANGE 0xFD20
#define C_GREY   0x8410

// ===== Sensors (I2C) =====
#define I2C_SDA  8
#define I2C_SCL  9
#define BMP_ADDR 0x77
Adafruit_AHTX0  aht;
Adafruit_BMP280 bmp;
bool ahtOK = false, bmpOK = false;

// ===== Time =====
const char* TIMEZONE = "GMT0BST,M3.5.0/1,M10.5.0/2";

// ===== Web Server =====
WebServer server(80);

// ===== History =====
#define HISTORY_SIZE 60
float tempHistory[HISTORY_SIZE];
float humHistory[HISTORY_SIZE];
float presHistory[HISTORY_SIZE];
int historyIndex = 0;
bool historyFilled = false;

// ===== Current readings =====
float temperature = NAN;
float humidity = NAN;
float pressure = NAN;
char timeStr[16] = "--:--:--";

// ===== Derived/system data =====
float avgTemperature = NAN;
float avgHumidity = NAN;
float avgPressure = NAN;
float dewPointC = NAN;

String tempTrend = "Steady";
String humTrend = "Steady";
String presTrend = "Steady";

unsigned long successfulUploads = 0;
unsigned long failedUploads = 0;
unsigned long wifiReconnectCount = 0;
unsigned long sensorErrorCount = 0;
unsigned long lastSuccessfulUploadMillis = 0;
unsigned long lastSuccessfulReadMillis = 0;

// ===== Firebase auth state =====
String firebaseIdToken = "";
String firebaseRefreshToken = "";
String firebaseLocalId = "";
unsigned long firebaseTokenTime = 0;

// ===== Timing =====
unsigned long lastUpload = 0;
unsigned long lastLog = 0;
unsigned long lastSensorRead = 0;
unsigned long lastScreenSwitch = 0;
unsigned long lastDraw = 0;

const unsigned long uploadInterval = 20000;           // 20 s
const unsigned long logInterval = 60000;              // 60 s
const unsigned long sensorInterval = 2000;            // 2 s
const unsigned long screenSwitchInterval = 6000;
const unsigned long drawInterval = 500;
const unsigned long authRefreshInterval = 3000000UL;  // 50 min

// ===== Screen mode =====
int screenMode = 0;
const int SCREEN_COUNT = 5;
bool screenEntered = true;

// ===== RTOS: mutex protecting shared sensor data, + network task handle =====
SemaphoreHandle_t dataMutex = NULL;
TaskHandle_t networkTaskHandle = NULL;

// ==========================
// Setup / Loop
// ==========================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n--- WeatherStation (round display) start ---");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  if (!gfx->begin()) Serial.println("gfx begin FAILED");

  // ---- boot splash: "CASE by Jasper (sourceonly)" ----
  gfx->fillScreen(C_BLACK);
  int16_t bx, by; uint16_t bw, bh;
  gfx->setTextColor(C_CYAN);
  gfx->setTextSize(4);
  gfx->getTextBounds("CASE", 0, 0, &bx, &by, &bw, &bh);
  gfx->setCursor((240 - bw) / 2, 78);
  gfx->print("CASE");

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->getTextBounds("by Jasper", 0, 0, &bx, &by, &bw, &bh);
  gfx->setCursor((240 - bw) / 2, 124);
  gfx->print("by Jasper");

  gfx->setTextColor(C_GREY);
  gfx->setTextSize(1);
  gfx->getTextBounds("(sourceonly)", 0, 0, &bx, &by, &bw, &bh);
  gfx->setCursor((240 - bw) / 2, 152);
  gfx->print("(sourceonly)");

  delay(1500);

  // ---- boot status ----
  gfx->fillScreen(C_BLACK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(60, 110);
  gfx->println("Starting...");

  // sensors
  Wire.begin(I2C_SDA, I2C_SCL);
  ahtOK = aht.begin();
  bmpOK = bmp.begin(BMP_ADDR);
  Serial.printf("AHT20: %s   BMP280: %s\n", ahtOK ? "ok" : "FAIL", bmpOK ? "ok" : "FAIL");

  for (int i = 0; i < HISTORY_SIZE; i++) {
    tempHistory[i] = 0;
    humHistory[i] = 0;
    presHistory[i] = 0;
  }

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  } else {
    if (!SPIFFS.exists("/log.csv")) {
      File file = SPIFFS.open("/log.csv", FILE_WRITE);
      if (file) {
        file.println("DateTime,Temperature,Humidity,Pressure");
        file.close();
      }
    }
  }

  syncTime();

  if (WiFi.status() == WL_CONNECTED) {
    firebaseAnonymousSignIn();
  }

  setupWebServer();

  // ===== RTOS: create the mutex, then start the network task =====
  dataMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    networkTask,          // function
    "networkTask",        // name
    8192,
    NULL,                 // params
    1,                    // priority
    &networkTaskHandle,   // handle
    0
  );
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (now - lastSensorRead >= sensorInterval) {
    lastSensorRead = now;
    readSensor();
  }

  if (now - lastLog >= logInterval) {
    lastLog = now;
    if (!isnan(temperature) && !isnan(humidity)) {
      logData(temperature, humidity, pressure);
    }
  }

  bool switched = false;
  if (now - lastScreenSwitch >= screenSwitchInterval) {
    lastScreenSwitch = now;
    screenMode++;
    if (screenMode >= SCREEN_COUNT) screenMode = 0;
    switched = true;
    screenEntered = true;
  }

  if (switched || now - lastDraw >= drawInterval) {
    lastDraw = now;

    if (screenMode == 0) drawMainScreen();
    else if (screenMode == 1) drawGauge("Temp",     temperature, -40, 85,   C_RED,    "C");
    else if (screenMode == 2) drawGauge("Humidity", humidity,      0, 100,  C_CYAN,   "%");
    else if (screenMode == 3) drawGauge("Pressure", pressure,    300, 1100, C_YELLOW, "hPa");
    else drawStatsScreen();

    screenEntered = false;
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}

// ==========================
// RTOS network task
// ==========================
void networkTask(void *param) {
  unsigned long tLastUpload = 0;
  unsigned long tLastAuth = 0;

  for (;;) {
    ensureWiFi();

    if (WiFi.status() == WL_CONNECTED) {
      unsigned long now = millis();

      if (now - tLastAuth >= 1000) {
        tLastAuth = now;
        ensureFirebaseAuth();
      }

      if (now - tLastUpload >= uploadInterval) {
        tLastUpload = now;

        float t, h;
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
          t = temperature;
          h = humidity;
          xSemaphoreGive(dataMutex);
        } else {
          vTaskDelay(pdMS_TO_TICKS(100));
          continue;
        }

        uploadCurrentToFirebase(t, h);
        pushHistoryToFirebase(t, h);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
