#include <WiFi.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>

// ------------------ RS485 / Modbus Config ------------------
#define RXD2 16
#define TXD2 17
#define DE_RE_PIN 4
const uint8_t SLAVE_ID = 1;

ModbusMaster node;

// ------------------ WiFi + ThingsBoard Config ------------------
const char* WIFI_SSID = "Redmi Note 12 Pro";
const char* WIFI_PASS = "12345677";
const char* MQTT_SERVER = "demo.thingsboard.io";
const int   MQTT_PORT = 1883;
const char* ACCESS_TOKEN = "PNxBUIagemCUTCKwYZ0j";

WiFiClient espClient;
PubSubClient client(espClient);

bool inverterOn = false; // tracked inverter state

// ------------------ Modbus Direction Control ------------------
void preTransmission() { digitalWrite(DE_RE_PIN, HIGH); delayMicroseconds(50); }
void postTransmission() { digitalWrite(DE_RE_PIN, LOW); delayMicroseconds(50); }

// ------------------ WiFi Connection ------------------
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

// ------------------ MQTT Connection ------------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (client.connect("ESP32_Inverter", ACCESS_TOKEN, NULL)) {
      Serial.println(" connected!");
      client.subscribe("v1/devices/me/attributes"); // listen for dashboard button changes
    } else {
      Serial.print(" failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ------------------ Callback: handle dashboard button ------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("Message [%s]: %s\n", topic, msg.c_str());

  if (msg.indexOf("\"state\":true") > 0) {
    inverterOn = true;
    node.writeSingleRegister(0, 1);  // inverter ON
    Serial.println("→ Inverter turned ON via dashboard");
  } 
  else if (msg.indexOf("\"state\":false") > 0) {
    inverterOn = false;
    node.writeSingleRegister(0, 0);  // inverter OFF
    Serial.println("→ Inverter turned OFF via dashboard");
  }

  // Report back immediately as shared attribute (sync dashboard)
  char statePayload[64];
  snprintf(statePayload, sizeof(statePayload), "{\"state\":%s}", inverterOn ? "true" : "false");
  client.publish("v1/devices/me/attributes", statePayload);
}

// ------------------ Modbus Setup ------------------
void setupModbus() {
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  node.begin(SLAVE_ID, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

// ------------------ Read Inverter ON/OFF ------------------
bool readInverterState() {
  uint8_t result = node.readHoldingRegisters(0, 1);
  if (result == node.ku8MBSuccess) {
    uint16_t val = node.getResponseBuffer(0);
    inverterOn = (val == 1);

    // publish actual state as shared attribute
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"state\":%s}", inverterOn ? "true" : "false");
    client.publish("v1/devices/me/attributes", payload);
    Serial.printf("Inverter state updated → %s\n", inverterOn ? "ON" : "OFF");
    return true;
  }
  Serial.printf("Failed to read inverter state (code: 0x%02X)\n", result);
  return false;
}

// ------------------ Send Telemetry to ThingsBoard ------------------
void sendTelemetry(float pv1v, float pv1i, float pv1p, float gridv, float gridi,
                   float gridFreq, float battv, int soc) {

  char payload[512];
  snprintf(payload, sizeof(payload),
           "{\"pv1_voltage\":%.1f, \"pv1_current\":%.1f, \"pv1_power\":%.1f, "
           "\"grid_voltage\":%.1f, \"grid_current\":%.1f, "
           "\"grid_frequency\":%.2f, \"battery_voltage\":%.1f, \"soc\":%d}",
           pv1v, pv1i, pv1p, gridv, gridi, gridFreq, battv, soc);

  if (client.publish("v1/devices/me/telemetry", payload))
    Serial.println("Telemetry sent: " + String(payload));
  else
    Serial.println("⚠️ Telemetry publish failed!");
}

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);
  connectWiFi();
  setupModbus();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  // Connect MQTT before sending anything
  reconnectMQTT();

  // --- Sync inverter state at startup ---
  if (readInverterState()) {
    Serial.printf("Initial inverter state: %s\n", inverterOn ? "ON" : "OFF");
  }
}

// ------------------ Loop ------------------
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  delay(1000);
  readInverterState();

  // --- Read main telemetry (0..124) ---
  delay(1000);
  uint8_t res = node.readInputRegisters(0, 125);
  if (res == node.ku8MBSuccess) {
    auto r = [&](uint16_t i)->uint16_t { return node.getResponseBuffer(i); };
    float pv1v = r(3) * 0.1;
    float pv1i = r(4) * 0.1;
    float pv1p = pv1v * pv1i;  // <-- Calculated PV1 Power (simple and effective)
    float gridv = r(38) * 0.1;
    float gridi = r(39) * 0.1;
    float gridFreq = r(37) * 0.01;

    float battv = 0;
    int soc = 0;

    // --- Battery info (1000..1040) ---
    delay(1000);
    res = node.readInputRegisters(1000, 41);
    if (res == node.ku8MBSuccess) {
      auto b = [&](uint16_t i)->uint16_t { return node.getResponseBuffer(i); };
      battv = b(13) * 0.1;
      soc = b(14);
    } else {
      Serial.printf("Battery read failed: 0x%02X\n", res);
    }

    sendTelemetry(pv1v, pv1i, pv1p, gridv, gridi, gridFreq, battv, soc);
  } 
  else {
    Serial.printf("Modbus read failed: 0x%02X\n", res);
  }

  delay(5000);
}
