#ifndef RELAY_HANDLER_H
#define RELAY_HANDLER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>

// Reference to mqttClient from mqtt_handler.h
extern PubSubClient mqttClient;

#define RELAY_PIN  5   // safe GPIO, active-low relay
bool lastRelayState = false;

// Init relay
void setupRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // OFF by default (active-low)
  lastRelayState = false;
}

// Get relay state
bool getRelayState() {
  return digitalRead(RELAY_PIN) == LOW;  // LOW = ON
}

// Send state to ThingsBoard (attribute)
void sendRelayStateAttribute() {
  bool relayState = getRelayState();
  if (relayState != lastRelayState) {
    lastRelayState = relayState;
    String payload = "{\"state\":" + String(relayState ? "true" : "false") + "}";
    mqttClient.publish("v1/devices/me/attributes", payload.c_str());
    Serial.println("Sent relay state attribute: " + payload);
  }
}

// Handle incoming TB attribute update
void handleRelayMessage(const String& message) {
  if (message.indexOf("state") != -1) {
    bool newState = message.indexOf("true") != -1;
    digitalWrite(RELAY_PIN, newState ? LOW : HIGH); // active-low logic
    Serial.println("Relay updated: " + String(newState ? "ON" : "OFF"));
    sendRelayStateAttribute();
  }
}

#endif
