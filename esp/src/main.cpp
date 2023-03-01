#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define DHTPIN    5
#define DHTTYPE   DHT11

#define SERVER_IP "1.1.1.1:1" // your server IP and Port here
#define STASSID "ssid"
#define STAPSK "password"

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

void setup() {
  Serial.begin(74880);

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());

  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);

  delay(sensor.min_delay / 1000);



  sensors_event_t event;
  float temperature = -1;
  float humidity = -1;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    temperature = event.temperature;
    Serial.println(F("°C"));
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    humidity = event.relative_humidity;
    Serial.println(F("%"));
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, "http://" SERVER_IP "/log");
  http.addHeader("Content-Type", "application/json");

  Serial.println("Posting");

  StaticJsonDocument<32> doc;

  doc["t"] = temperature;
  doc["h"] = humidity;
  byte buffer[32];

  size_t size = serializeJson(doc, &buffer, 32);
  http.POST(&buffer[0], size);

  http.end();

  Serial.println("SLEEPING");
  ESP.deepSleep(60e6);
}

void loop() {

}