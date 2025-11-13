#ifndef FOTA_HANDLER_H
#define FOTA_HANDLER_H

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include "cert.h"
#include "config.h"

String newFwVersion = "";
String newFwBinURL = "";
unsigned long previousMillis_update = 0;
const long update_interval = 30000; // 30s

void firmwareUpdate() {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  t_httpUpdate_return ret = httpUpdate.update(client, newFwBinURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No updates available.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update successful!");
      break;
  }
}

int FirmwareVersionCheck() {
  String payload;
  int httpCode;
  String FirmwareURL = URL_fw_JSON;
  FirmwareURL += "?";
  FirmwareURL += String(rand());

  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  HTTPClient https;

  if (https.begin(client, FirmwareURL)) {
    httpCode = https.GET();
    if (httpCode == HTTP_CODE_OK) {
      payload = https.getString();
    } else {
      Serial.printf("Error on GET: %d\n", httpCode);
    }
    https.end();
  }

  if (httpCode == HTTP_CODE_OK) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload)) {
      Serial.println("JSON parse failed!");
      return 0;
    }
    newFwVersion = doc["esp32"]["version"].as<String>();
    newFwBinURL = doc["esp32"]["bin_url"].as<String>();

    if (newFwVersion.equals(FirmwareVer)) {
      Serial.println("Already latest firmware.");
      return 0;
    } else {
      Serial.printf("New firmware found: %s\n", newFwVersion.c_str());
      return 1;
    }
  }
  return 0;
}

void checkForUpdate() {
  if (FirmwareVersionCheck()) {
    firmwareUpdate();
  }
}

#endif
