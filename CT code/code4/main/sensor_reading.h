#ifndef SENSOR_READING_H
#define SENSOR_READING_H

#include <EmonLib.h>

const int CT_PIN = 32;
const float CT_CALIBRATION = 6;
const float VOLTAGE_MIN = 230.0;
const float VOLTAGE_MAX = 245.0;
const float POWER_FACTOR = 0.71;

struct SensorData {
  float current;
  float voltage;
  float realPower;
  float apparentPower;
  float powerFactor;
  unsigned long timestamp;
};

SensorData latestData;
EnergyMonitor emon1;

void setupEmonLib() {
  emon1.current(CT_PIN, CT_CALIBRATION);
  analogReadResolution(12);
  randomSeed(analogRead(0));
  latestData.powerFactor = POWER_FACTOR;
}

void readAndCalculateData() {
  latestData.current = emon1.calcIrms(1480);
  latestData.voltage = random(VOLTAGE_MIN * 100, VOLTAGE_MAX * 100 + 1) / 100.0;
  latestData.apparentPower = latestData.current * latestData.voltage;
  latestData.realPower = latestData.apparentPower * latestData.powerFactor;
  latestData.timestamp = millis();

  latestData.current = round(latestData.current * 1000) / 1000.0;
  latestData.voltage = round(latestData.voltage * 100) / 100.0;
  latestData.realPower = round(latestData.realPower * 100) / 100.0;
  latestData.apparentPower = round(latestData.apparentPower * 100) / 100.0;

  Serial.println("=== Sensor Data ===");
  Serial.println("Current: " + String(latestData.current));
  Serial.println("Voltage: " + String(latestData.voltage));
  Serial.println("Real Power: " + String(latestData.realPower));
  Serial.println("Apparent Power: " + String(latestData.apparentPower));
  Serial.println("===================");
}

#endif
