#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <time.h>

// Firestore Configuration
const char* firestoreUrl = "https://firestore.googleapis.com/v1/projects/mbc-firebaseinteg/databases/(default)/documents/sensordataniMacailao?key=AIzaSyCApbXmJxXmlwHAaJXxkgYT-dtAyRnibMo";

// Wi-Fi Credentials
const char* ssid = "TITI";
const char* password = "1234567891";

// NTP Server Configuration
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = 8 * 3600; // GMT+8 for Philippines
const int daylightOffset_sec = 0;

// DHT Sensor Configuration
#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Photoresistor Configuration
const int photoresistorPin = 34;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Start DHT Sensor
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure NTP Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  // Ensure Wi-Fi Connection
  if (WiFi.status() == WL_CONNECTED) {
    // Read data from DHT Sensor
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if the readings are valid
    if (isnan(temp) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Read data from Photoresistor
    int photoresistorValue = analogRead(photoresistorPin);
    int daylight = (photoresistorValue > 512) ? 1 : 0; // Example threshold: Adjust the value as needed


    // Get the current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }

    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // Prepare HTTP Client
    HTTPClient http;
    http.begin(firestoreUrl);
    http.addHeader("Content-Type", "application/json");

    // JSON Payload for Firestore
    String jsonData = String("{") +
                      "\"fields\": {" +
                      "\"temperature\": {\"stringValue\": \"" + String(temp, 1) + "\"}," +
                      "\"humidity\": {\"stringValue\": \"" + String(humidity, 1) + "\"}," +
                      "\"daylight\": {\"stringValue\": \"" + String(daylight) + "\"}," +
                      "\"timestamp\": {\"stringValue\": \"" + String(timestamp) + "\"}" +
                      "}" +
                      "}";

    // Send POST Request
    int httpResponseCode = http.POST(jsonData);

    // Log HTTP Response
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + http.getString());
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    http.end();
  } else {
    // Handle Wi-Fi Disconnection
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    WiFi.reconnect();
  }

  // Delay before next loop
  delay(5000);
}

