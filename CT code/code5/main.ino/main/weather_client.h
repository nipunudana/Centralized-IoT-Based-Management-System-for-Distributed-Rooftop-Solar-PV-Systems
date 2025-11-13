#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ====== CONFIG ======
static const double OM_LAT = 7.2986;
static const double OM_LON = 81.8547;

// Pull weather every 10 minutes (adjust if you want)
static const unsigned long WEATHER_FETCH_INTERVAL_MS = 10UL * 60UL * 1000UL;

// ====== STATE ======
struct WeatherData {
  float ghi;               // shortwave_radiation (GHI), W/m² (avg last hour)
  float temperature;       // °C
  float humidity;          // %
  float windSpeed;         // km/h (Open-Meteo default)
  float uvIndex;           // unitless
  float precipitation;     // mm
  float cloudCover;        // % total
  float cloudCoverLow;     // % low
  String timeISO;          // current.time
  bool   valid;            // whether last fetch succeeded
};

static WeatherData gWeather = {NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,"",false};
static unsigned long gLastWeatherFetch = 0;

// ====== INTERNAL: Build URL once ======
inline String buildOpenMeteoURL() {
  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(OM_LAT, 6);
  url += "&longitude="; url += String(OM_LON, 6);
  url += "&timezone=auto";
  // Use CURRENT to avoid huge hourly arrays and night-time index issues
  url += "&current=shortwave_radiation,cloud_cover_low,cloud_cover,temperature_2m,relative_humidity_2m,wind_speed_10m,uv_index,precipitation";
  return url;
}

// ====== PUBLIC: Try fetch now (rate-limited) ======
inline bool weather_fetch_if_due(bool force = false) {
  const unsigned long now = millis();
  if (!force && (now - gLastWeatherFetch) < WEATHER_FETCH_INTERVAL_MS) return gWeather.valid;

  const String url = buildOpenMeteoURL();
  WiFiClientSecure client;
  client.setInsecure(); // ESP32: accept default certs without bundle

  HTTPClient http;
  bool ok = false;

  if (http.begin(client, url)) {
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      // Filter only the fields we care about to save RAM
      StaticJsonDocument<256> filter;
      JsonObject fcur = filter.createNestedObject("current");
      fcur["time"] = true;
      fcur["shortwave_radiation"] = true;
      fcur["temperature_2m"] = true;
      fcur["relative_humidity_2m"] = true;
      fcur["wind_speed_10m"] = true;
      fcur["uv_index"] = true;
      fcur["precipitation"] = true;
      fcur["cloud_cover"] = true;
      fcur["cloud_cover_low"] = true;

      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      if (!err) {
        JsonObject cur = doc["current"];
        // Note: Some variables can legitimately be 0 at night (e.g., shortwave_radiation, uv_index)
        gWeather.timeISO       = cur["time"] | "";
        gWeather.ghi           = cur["shortwave_radiation"].isNull() ? NAN : cur["shortwave_radiation"].as<float>();
        gWeather.temperature   = cur["temperature_2m"].isNull()      ? NAN : cur["temperature_2m"].as<float>();
        gWeather.humidity      = cur["relative_humidity_2m"].isNull()? NAN : cur["relative_humidity_2m"].as<float>();
        gWeather.windSpeed     = cur["wind_speed_10m"].isNull()      ? NAN : cur["wind_speed_10m"].as<float>();
        gWeather.uvIndex       = cur["uv_index"].isNull()            ? NAN : cur["uv_index"].as<float>();
        gWeather.precipitation = cur["precipitation"].isNull()       ? NAN : cur["precipitation"].as<float>();
        gWeather.cloudCover    = cur["cloud_cover"].isNull()         ? NAN : cur["cloud_cover"].as<float>();
        gWeather.cloudCoverLow = cur["cloud_cover_low"].isNull()     ? NAN : cur["cloud_cover_low"].as<float>();
        gWeather.valid = true;
        ok = true;
      } else {
        Serial.printf("[weather] JSON parse error: %s\n", err.c_str());
        gWeather.valid = false;
      }
    } else {
      Serial.printf("[weather] HTTP GET failed, code=%d\n", code);
      gWeather.valid = false;
    }
    http.end();
  } else {
    Serial.println("[weather] http.begin() failed");
    gWeather.valid = false;
  }

  gLastWeatherFetch = now;
  return ok;
}

// ====== PUBLIC: Access latest (may contain NANs) ======
inline WeatherData weather_latest() {
  return gWeather;
}

#endif // WEATHER_CLIENT_H
