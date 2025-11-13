#include "wifi_setup.h"
#include "sensor_reading.h"
#include "google_sheets.h"
#include "relay_handler.h"
#include "mqtt_handler.h"
#include "fota_handler.h"
#include "config.h"


void setup() {
  Serial.begin(115200);
  Serial.print("Active Firmware Version: ");
  Serial.println(FirmwareVer);

  setup_wifi();
  setupEmonLib();
  setupRelay();
  setup_mqtt();

  Serial.println("System Started!");
}

void loop() {
  // Maintain MQTT connection + telemetry
  maintain_mqtt();

  // Google Sheets logging
  if (millis() - lastGoogleSheetsTime >= GOOGLE_SHEETS_INTERVAL) {
    sendDataToGoogleSheets();
    lastGoogleSheetsTime = millis();
  }

  // FOTA check every 30s
  if (millis() - previousMillis_update >= update_interval) {
    previousMillis_update = millis();
    checkForUpdate();
  }
}
