#include "wifi_setup.h"
#include "sensor_reading.h"
#include "google_sheets.h"
#include "relay_handler.h"
#include "mqtt_handler.h"

void setup() {
  Serial.begin(115200);
  setup_wifi();     // now WiFi is handled here
  setupEmonLib();
  setupRelay();
  setup_mqtt();
}

void loop() {
  maintain_mqtt();

  // Send data to Google Sheets every minute
  if (millis() - lastGoogleSheetsTime >= GOOGLE_SHEETS_INTERVAL) {
    sendDataToGoogleSheets();
    lastGoogleSheetsTime = millis();
  }
}
