#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>

#define ss 5
#define rst 2
#define dio0 4

const char* ssid = "iPhone de Nico";
const char* pass = "12345689";

#define TB_SERVER "thingsboard.cloud"
#define TOKEN "BMBDsh59a2qMMiuonu5Z"

constexpr uint16_t MAX_MESSAGE_SIZE = 128U;

WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi.");
  } else {
    Serial.println("\nConnected to WiFi");
  }
}

void connectToThingsBoard() {
  if (!tb.connected()) {
    Serial.println("Connecting to ThingsBoard server");

    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
    } else {
      Serial.println("Connected to ThingsBoard");
    }
  }
}

void sendDataToThingsBoard(float lat, float lon, float speed) {
  String jsonData = "{\"latitude\":" + String(lat, 6) + ", \"longitude\":" + String(lon, 6) + ", \"speed\":" + String(speed, 2) + "}";
  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard: " + jsonData);
}

void setup() {
  Serial.begin(115200);
  Serial.println("LoRa Receiver");

  LoRa.setPins(ss, rst, dio0);    //setup LoRa transceiver module
  if (!LoRa.begin(433E6)){     //433E6 - Asia, 866E6 - Europe, 915E6 - North America
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  connectToWiFi();
  connectToThingsBoard();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.println("Received packet:");

    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    int latIndex = incoming.indexOf("LAT:");
    int lonIndex = incoming.indexOf(",LONG:");
    int speedIndex = incoming.indexOf(",SPEED:");

    float lat = incoming.substring(latIndex + 4, lonIndex).toFloat();
    float lon = incoming.substring(lonIndex + 6, speedIndex).toFloat();
    float speed = incoming.substring(speedIndex + 7).toFloat();

    String latString = String(lat, 6);
    String lonString = String(lon, 6);
    String speedString = String(speed, 2) + " kmph";

    Serial.print("Latitude: ");
    Serial.println(latString);
    Serial.print("Longitude: ");
    Serial.println(lonString);
    Serial.print("Speed: ");
    Serial.println(speedString);

    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());

    if (!tb.connected()) {
      connectToThingsBoard();
    }

    sendDataToThingsBoard(lat, lon, speed);
  }

  tb.loop();
}
