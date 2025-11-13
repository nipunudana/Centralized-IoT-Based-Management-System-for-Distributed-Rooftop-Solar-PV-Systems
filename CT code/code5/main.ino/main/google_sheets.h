#ifndef GOOGLE_SHEETS_H
#define GOOGLE_SHEETS_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "sensor_reading.h"

const char* GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbzWrWQJlzD5vdbXgqYiEz3IATdf1HSN9DdegnwGE9duaER73u8KOvJST5oPqsF0bsKo/exec";

unsigned long lastGoogleSheetsTime = 0;
const unsigned long GOOGLE_SHEETS_INTERVAL = 60000;

void sendDataToGoogleSheets() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  HTTPClient http;
  http.begin(GOOGLE_SCRIPT_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<400> doc;
  doc["output_current"] = latestData.current;
  doc["output_voltage"] = latestData.voltage;
  doc["output_power"] = latestData.realPower;
  doc["apparent_power"] = latestData.apparentPower;

  String payload;
  serializeJson(doc, payload);
  Serial.println("Sending to Google Sheets:");
  Serial.println(payload);

  int responseCode = http.POST(payload);

  if (responseCode > 0) {
    Serial.println("✓ Response Code: " + String(responseCode));
    Serial.println("Response: " + http.getString());
  } else {
    Serial.println("✗ Failed to send. Error code: " + String(responseCode));
  }

  http.end();
}

#endif
