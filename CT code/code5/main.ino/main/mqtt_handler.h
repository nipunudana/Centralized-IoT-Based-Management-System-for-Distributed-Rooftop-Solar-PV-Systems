#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "sensor_reading.h"
#include "relay_handler.h"
#include "weather_client.h"   // <-- NEW

// ===== ThingsBoard =====
const char* MQTT_HOST = "demo.thingsboard.io";
const int   MQTT_PORT = 1883;
const char* ACCESS_TOKEN = "PNxBUIagemCUTCKwYZ0j"; // your device token

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Timers
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 5000; // 5s for your electrical data

// ===== MQTT callback: Device-side attribute updates (e.g., relay control) =====
void callback(char* topic, byte* payload, unsigned int length) {
  String message; message.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  Serial.println(String("[mqtt] Msg on ") + topic + ": " + message);
  handleRelayMessage(message);
}

// ===== Setup MQTT (assumes WiFi ready) =====
void setup_mqtt() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}

// ===== Reconnect loop =====
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[mqtt] Connecting...");
    // Use device token as password, no user
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

// ===== Publish telemetry to ThingsBoard =====
void publishTelemetry() {
  // 1) Update electrical data
  readAndCalculateData();

  // 2) Rate-limited weather refresh (non-blocking most of the time)
  weather_fetch_if_due(false);
  WeatherData w = weather_latest();

  // 3) Build JSON (keep small -> StaticJsonDocument<512> is enough)
  StaticJsonDocument<512> doc;

  // Electrical
  doc["output_current"]   = latestData.current;
  doc["output_voltage"]   = latestData.voltage;
  doc["output_power"]     = latestData.realPower;
  doc["apparent_power"]   = latestData.apparentPower;

  // Weather (send even if 0; omit if NAN)
  if (!isnan(w.ghi))           doc["ghi"]            = w.ghi;               // W/m²
  if (!isnan(w.temperature))   doc["temp_c"]         = w.temperature;       // °C
  if (!isnan(w.humidity))      doc["rh"]             = w.humidity;          // %
  if (!isnan(w.windSpeed))     doc["wind_kmh"]       = w.windSpeed;         // km/h
  if (!isnan(w.uvIndex))       doc["uv_index"]       = w.uvIndex;
  if (!isnan(w.precipitation)) doc["precip_mm"]      = w.precipitation;     // mm
  if (!isnan(w.cloudCover))    doc["cloud_cover"]    = w.cloudCover;        // %
  if (!isnan(w.cloudCoverLow)) doc["cloud_cover_low"]= w.cloudCoverLow;     // %
  if (w.valid && w.timeISO.length()) doc["weather_time"] = w.timeISO;       // ISO8601

  // 4) Serialize & publish
  char payload[600];
  size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0) {
    Serial.println("[mqtt] JSON serialize failed (buffer too small?)");
    return;
  }

  const char* topic = "v1/devices/me/telemetry";
  bool ok = mqttClient.publish(topic, payload, false);
  Serial.printf("[mqtt] Telemetry (%u bytes) %s: %s\n", (unsigned)n, ok ? "sent" : "FAILED", payload);

  // Keep relay attribute synced
  sendRelayStateAttribute();
}

// ===== Loop handler =====
void maintain_mqtt() {
  if (!mqttClient.connected()) reconnect();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    lastTelemetryTime = now;
    publishTelemetry();
  }
}

#endif // MQTT_HANDLER_H
