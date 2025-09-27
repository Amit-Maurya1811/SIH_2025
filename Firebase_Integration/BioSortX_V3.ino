#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

// ==== Load Cell Pins ====
const int HX711_1_dout = 4;
const int HX711_1_sck  = 23;

const int HX711_2_dout = 16;
const int HX711_2_sck  = 17;

const int HX711_3_dout = 18;
const int HX711_3_sck  = 19;

HX711_ADC LoadCell1(HX711_1_dout, HX711_1_sck);
HX711_ADC LoadCell2(HX711_2_dout, HX711_2_sck);
HX711_ADC LoadCell3(HX711_3_dout, HX711_3_sck);

const int calValAddr_1 = 0;
const int calValAddr_2 = calValAddr_1 + sizeof(float);
const int calValAddr_3 = calValAddr_2 + sizeof(float);

float w1;
float w2;
float w3;

// ==== WiFi & Firebase ====
#define WIFI_SSID "Amit"
#define WIFI_PASSWORD "manmit88"
#define st 22

#define FIREBASE_HOST "https://biosortx-c2820-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyCovEJjcZlpdjzM3E4zJwvSYEI5jFu068c"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
const unsigned long sendInterval = 5000; // 5 seconds

String timestamp = "unknown_time";
String logPath = "";

// ---- FUNCTION: Start Load Cell ----
void startLoadCell(HX711_ADC &cell, int addr, const char* name) {
  cell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  cell.start(stabilizingtime, _tare);
  if (cell.getTareTimeoutFlag() || cell.getSignalTimeoutFlag()) {
    Serial.print(name);
    Serial.println(": Timeout, check wiring!");
    while (1);
  } else {
    float calVal;
    EEPROM.get(addr, calVal);
    if (isnan(calVal)) calVal = 1.0;
    cell.setCalFactor(calVal);
    Serial.print(name);
    Serial.print(" started. Calibration factor: ");
    Serial.println(calVal);
  }
  while (!cell.update());
}

// ---- FUNCTION: Get Timestamp ----
String getTimestamp() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "unknown_time";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  return String(buf);
}

// ---- FUNCTION: Update Firebase ----
void update_firebase() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > sendInterval)) {
    sendDataPrevMillis = millis();

    // Update load cells
    LoadCell1.update();
    LoadCell2.update();
    LoadCell3.update();

    float w1 = LoadCell1.getData();
    float w2 = LoadCell2.getData();
    float w3 = LoadCell3.getData();

    // Print on Serial Monitor
    Serial.print("LC1: "); Serial.print(w1);
    Serial.print("  LC2: "); Serial.print(w2);
    Serial.print("  LC3: "); Serial.println(w3);


    logPath = "model_001/" + timestamp;

    // Push data to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, logPath + "/LC1", w1) &&
        Firebase.RTDB.setFloat(&fbdo, logPath + "/LC2", w2) &&
        Firebase.RTDB.setFloat(&fbdo, logPath + "/LC3", w3)) {
      Serial.println("Load cell data updated to Firebase ✅");
    } else {
      Serial.println("Firebase update failed: " + fbdo.errorReason());
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(st, INPUT_PULLUP);
  EEPROM.begin(512);
  startLoadCell(LoadCell1, calValAddr_1, "LoadCell 1");
  startLoadCell(LoadCell2, calValAddr_2, "LoadCell 2");
  startLoadCell(LoadCell3, calValAddr_3, "LoadCell 3");

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

  // Time setup
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Firebase setup
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign-up successful.");
  } else {
    Serial.printf("Firebase sign-up failed: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  while (timestamp == "unknown_time") {
    timestamp = getTimestamp();
  }
  logPath = "model_001/" + timestamp;
}

//waste collection
void waste_collection()
{
  // Add log entry with timestamp
       timestamp = getTimestamp();
       Serial.print("Time stamp updated");
       logPath = "model_001/" + timestamp;
      // Push data to Firebase
    w1=0.0;
    w2=0.0;
    w3=0.0;
}

void loop() {
    LoadCell1.update();
    LoadCell2.update();
    LoadCell3.update();
  if( digitalRead(st) == HIGH)
  {
    waste_collection();
  }
  else{
    update_firebase(); // every 5 sec
  }
}
