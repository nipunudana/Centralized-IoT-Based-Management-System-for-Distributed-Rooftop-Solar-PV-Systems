#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sensor_reading.h"
#include "relay_handler.h"


// ThingsBoard credentials
const char* MQTT_HOST = "demo.thingsboard.io";
const int MQTT_PORT = 1883;
const char* ACCESS_TOKEN = "PNxBUIagemCUTCKwYZ0j";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 5000;

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  Serial.println("MQTT message: " + message);

  // Pass to relay handler
  handleRelayMessage(message);
}

// Setup MQTT (assumes WiFi already connected)
void setup_mqtt() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}

// Reconnect if needed
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP32Client", ACCESS_TOKEN, NULL)) {
      Serial.println("connected.");
      mqttClient.subscribe("v1/devices/me/attributes");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

// Publish sensor telemetry
void publishTelemetry() {
  readAndCalculateData();

  StaticJsonDocument<400> doc;
  doc["output_current"] = latestData.current;
  doc["output_voltage"] = latestData.voltage;
  doc["output_power"] = latestData.realPower;
  doc["apparent_power"] = latestData.apparentPower;

  String payload;
  serializeJson(doc, payload);

  mqttClient.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Telemetry sent: " + payload);
}

// Loop handler
void maintain_mqtt() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastTelemetryTime > TELEMETRY_INTERVAL) {
    lastTelemetryTime = now;
    publishTelemetry();
    sendRelayStateAttribute(); // keep relay synced
  }
}

#endif
