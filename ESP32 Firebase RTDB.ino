#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
//#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "time.h"

#define API_KEY       "AIzaSyCnqizfZOBY3_s-JcZnSZ9Q0Xn5GdBEB8Q"
#define USER_EMAIL    "evanalifwidhyatma@gmail.com"
#define USER_PASSWORD "210703"
#define DATABASE_URL  "https://jerukagung-meteorologi-default-rtdb.asia-southeast1.firebasedatabase.app/"

const char* ssid     = "Jerukagung Seismologi";   // your network SSID (name)
const char* password = "seiscalogi";   // your network password
const char* ntpServer = "time.google.com";

//Firebase Lamp Indicator
const int ledStat = 2;
const int ledWifi = 4;
const int ledGood = 16;
const int ledFail = 17;

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;
// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temp";
//String humPath = "/humidity";
String presPath = "/press";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;
int timestamp;
FirebaseJson json;

// BMP280 sensor
Adafruit_BMP280 bmp; // I2C
float temperature;
//float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long CountMillis = 0;
unsigned long timerDelay = 15000;

// Initialize BMP280
void initBMP() {
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi....");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  pinMode(ledStat,OUTPUT);
  initBMP();
  initWiFi();
  configTime(0, 0, ntpServer);
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/data/meteorologi";
}

void FirebaseSend() {
  // Send new readings to database
  if (Firebase.ready() && (millis() - CountMillis > timerDelay || CountMillis == 0)) {
    digitalWrite(ledStat,HIGH);
    timestamp = getTime();
    Serial.print ("Time: ");
    Serial.println (timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(), String(bmp.readTemperature()));
    //json.set(humPath.c_str(), String(bme.readHumidity()));
    json.set(presPath.c_str(), String(bmp.readPressure() / 100.0F));
    json.set(timePath, String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    CountMillis = millis();
    digitalWrite(ledStat,LOW);
  }
}
void loop() {
  FirebaseSend();
}
