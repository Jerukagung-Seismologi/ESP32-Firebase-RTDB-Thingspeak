#include <WiFi.h>
#include <WiFiClient.h>
#include <Firebase_ESP_Client.h>
//#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "time.h"
#include "ThingSpeak.h"

#define API_KEY       "YOUR API KEY"
#define USER_EMAIL    "YOUR AUTHENTICATION EMAIL"
#define USER_PASSWORD "YOUR AUTHENTICATION PASSWORD"
#define DATABASE_URL  "YOUR DATABASE URL/"

const char* ssid      = "Jerukagung Seismologi";   // your network SSID (name)
const char* password  = "seisca";   // your network password
const char* ntpServer = "time.google.com";

WiFiClient client;

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
String tempPath  = "/temp";
//String humPath = "/humidity";
String presPath  = "/press";
String dewPath   = "/dew";
String hidePath  = "/hide";
String timePath  = "/timestamp";
String heapPath  = "/espheapram";

// Parent Node (to be updated in every loop)
String parentPath;
int timestamp;
int espheapram;
FirebaseJson json;

// BMP280 sensor
Adafruit_BMP280 bmp; // I2C
float temperature;
//float humidity;
float pressure;

// Timer variables (send new readings every three minutes)
unsigned long CountMillis = 0;
unsigned long FirebaseDelay = 15000;

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
  ThingSpeak.begin(client);
  configTime(0, 0, ntpServer); //NTP Server
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
  Serial.print("User UID: "); Serial.println(uid);

  // Update database path
  databasePath = "/data/meteorologi";
}

void FirebaseSend() {
  // Send new readings to database
  if (Firebase.ready() && (millis() - CountMillis > FirebaseDelay || CountMillis == 0)) {
    digitalWrite(ledStat,HIGH);
    timestamp = getTime();
    espheapram = ESP.getFreeHeap();
    Serial.print ("Time: ");    Serial.println (timestamp);
    Serial.print ("HeapRAM: "); Serial.println (espheapram);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(tempPath.c_str(),  String(bmp.readTemperature()));
    //json.set(humPath.c_str(), String(bme.readHumidity()));
    json.set(presPath.c_str(),  String(bmp.readPressure() / 100.0F));
    json.set(heapPath,          String(espheapram));
    json.set(timePath,          String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    CountMillis = millis();
    digitalWrite(ledStat,LOW);
    Serial.println(ESP.getFreeHeap());
  }
}

// Timer variables
unsigned int myChannelNumber = YOUR NUMBER CHANNEL ;
const char* myWriteAPIKey = "YOUR WRITE API KEY";
unsigned long lastTime = 0;
unsigned long thingspeakDelay = 59000;

void Thingspeak() {
  if ((millis() - lastTime) > thingspeakDelay) {
    // Get a new Total reading
    float temperatureTot = bmp.readTemperature();
    Serial.print("Temperature (ºC): ");
    Serial.println(temperatureTot);
    /*humidityTot = htu.readHumidity();
      Serial.print("Humidity (%): ");
      Serial.println(humidityTot);*/
    float pressureTot = bmp.readPressure() / 100.0F;
    Serial.print("Pressure (hPa): ");
    Serial.println(pressureTot);

    // set the fields with the values
    ThingSpeak.setField(1, temperatureTot);
    //ThingSpeak.setField(2, humidityTot);
    ThingSpeak.setField(3, pressureTot);

    //HTTP Kode untuk mengecek berhasil atau gagal
    int http = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
               ThingSpeak.setStatus(String(http));

    if (http == 200) {
      //digitalWrite(ledGood, HIGH);
      //digitalWrite(ledFail, LOW);
      Serial.println("Data Berhasil Update. Kode HTTP" + String(http));
    }
    else {
      //digitalWrite(ledFail, HIGH);
      //digitalWrite(ledGood, LOW);
      Serial.println("Data Gagal Update. Kode HTTP" + String(http));
    }
    lastTime = millis();
    Serial.println(ESP.getFreeHeap());
  }
}
void loop() {
  FirebaseSend();
  Thingspeak();
}
