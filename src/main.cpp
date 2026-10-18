#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHTesp.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

const char *API_URL = "http://iot-alliyah.northcentralus.cloudapp.azure.com";

const unsigned long WIFI_TIMEOUT_MS = 15000;
const int DHT_PIN = 15;
const unsigned long SEND_EVERY_MS = 10000;

DHTesp dht;
unsigned long lastSend = 0;

bool connectWiFi() {
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println();
      Serial.println("No Wi-Fi after 15 s. Check the name and the password.");
      return false;
    }

    delay(250);
    Serial.print(".");
  }

  Serial.println(" connected");
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  return true;
}

void checkServer() {
  HTTPClient http;
  http.begin(String(API_URL) + "/devices");
  int status = http.GET();

  Serial.print("GET /devices -> ");
  Serial.println(status);

  if (status > 0) {
    Serial.println(http.getString());
  } else {
    Serial.println(http.errorToString(status));
  }

  http.end();
}

void sendReading(float temp) {
  char body[80];

  snprintf(body, sizeof(body), "{\"mac\": \"%s\", \"temp\": %.1f}",
           WiFi.macAddress().c_str(), temp);

  HTTPClient http;
  http.begin(String(API_URL) + "/readings");
  http.addHeader("Content-Type", "application/json");
  int status = http.POST(body);

  Serial.print("POST ");
  Serial.print(body);
  Serial.print(" -> ");
  Serial.println(status);

  if (status == 404) {
    Serial.println("Unknown MAC: register this board with POST /devices first.");
  } else if (status < 0) {
    Serial.println(http.errorToString(status));
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello from the ESP32");

  dht.setup(DHT_PIN, DHTesp::DHT22);

  if (connectWiFi()) {
    checkServer();
  }
}

void loop() {
  if (millis() - lastSend < SEND_EVERY_MS) {
    return;
  }

  lastSend = millis();

  float temp = dht.getTemperature();

  if (isnan(temp)) {
    Serial.println("No reading from the sensor. Check the three wires.");
    return;
  }

  Serial.print("Temperature: ");
  Serial.println(temp);

  sendReading(temp);
}