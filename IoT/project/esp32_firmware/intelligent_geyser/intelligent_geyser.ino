// ============================================================================
// intelligent_geyser.ino — Context-Aware Smart Water Geyser (ESP32 Firmware)
// ============================================================================
// This firmware operates in two phases:
//   Phase 1 (INIT):  Registers with the cloud server using its MAC address.
//   Phase 2 (DATA):  Reads sensors, publishes telemetry, receives ON/OFF commands.
//   FALLBACK:        If WiFi/MQTT is lost, relay follows its current physical state (manual mode).
//
// Sensors: DS18B20 (water temp), DHT22 (ambient temp+humidity), YF-S201 (flow),
//          ACS712 (current), 2x XKC-Y25 (water level: HIGH/MEDIUM/LOW)
//
// Target Board: ESP32-D0WD-V3 (38-pin DevKit)
// ============================================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include "config.h"

// ── Global Objects ──────────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient mqttClient(espClient);

OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

DHT dht(DHT22_PIN, DHT22);

// ── State Variables ─────────────────────────────────────────────────────────
enum DevicePhase { PHASE_INIT, PHASE_DATA };
DevicePhase currentPhase = PHASE_INIT;

String deviceMAC_clean = "";
String deviceMAC_colon = "";
int assignedGeyserID = -1;
String dataTopic = "";
String commandTopic = "";

// Sensor readings
float waterTemp = 0.0;
float ambientTemp = 0.0;
float humidity = 0.0;
float flowRate = 0.0;
float currentAmps = 0.0;
bool waterLevelHigh = false;   // true if water reaches HIGH sensor
bool waterLevelLow = false;    // true if water reaches LOW sensor
String waterLevelStr = "LOW";  // "HIGH", "MEDIUM", or "LOW"
bool heaterOn = false;
bool serverConnected = false;

// Flow sensor interrupt
volatile unsigned long pulseCount = 0;
unsigned long lastFlowCalc = 0;

// Timing
unsigned long lastPublish = 0;
unsigned long lastSensorPrint = 0;
unsigned long initSentTime = 0;
bool initSent = false;

// ── Interrupt Service Routine (Flow Sensor) ─────────────────────────────────
void IRAM_ATTR flowPulseISR() {
  pulseCount++;
}

// ── WiFi Connection ─────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    deviceMAC_colon = WiFi.macAddress();
    deviceMAC_clean = deviceMAC_colon;
    deviceMAC_clean.replace(":", "");
    Serial.println("MAC: " + deviceMAC_colon);
  } else {
    Serial.println("\nWiFi connection FAILED. Entering manual mode.");
  }
}

// ── MQTT Callback ───────────────────────────────────────────────────────────
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Parse incoming JSON
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.print("JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  String topicStr = String(topic);

  // ── Handle config response during INIT phase ───────────────────────────
  if (currentPhase == PHASE_INIT) {
    String expectedConfigTopic = String(BASE_TOPIC) + "/config/" + deviceMAC_colon;
    if (topicStr == expectedConfigTopic) {
      assignedGeyserID = doc["geyser_id"] | -1;
      const char* dt = doc["data_topic"] | "";
      const char* ct = doc["command_topic"] | "";
      dataTopic = String(dt);
      commandTopic = String(ct);

      Serial.println("=== INIT COMPLETE ===");
      Serial.println("Assigned ID: " + String(assignedGeyserID));
      Serial.println("Data Topic:  " + dataTopic);
      Serial.println("Cmd Topic:   " + commandTopic);

      // Subscribe to our command topic
      mqttClient.subscribe(commandTopic.c_str());
      Serial.println("Subscribed to: " + commandTopic);

      currentPhase = PHASE_DATA;
      serverConnected = true;
    }
    return;
  }

  // ── Handle ON/OFF commands during DATA phase ───────────────────────────
  if (currentPhase == PHASE_DATA && topicStr == commandTopic) {
    const char* cmd = doc["command"] | "OFF";
    if (strcmp(cmd, "ON") == 0) {
      heaterOn = true;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("[CMD] Heater ON");
    } else {
      heaterOn = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("[CMD] Heater OFF");
    }
  }
}

// ── MQTT Connection ─────────────────────────────────────────────────────────
void connectMQTT() {
  if (mqttClient.connected()) return;

  String clientId = "geyser_" + deviceMAC_clean;
  Serial.print("Connecting to MQTT...");

  if (mqttClient.connect(clientId.c_str())) {
    Serial.println(" connected!");

    if (currentPhase == PHASE_INIT) {
      // Subscribe to our config channel
      String configTopic = String(BASE_TOPIC) + "/config/" + deviceMAC_colon;
      mqttClient.subscribe(configTopic.c_str());
      Serial.println("Subscribed to: " + configTopic);
    } else if (currentPhase == PHASE_DATA && commandTopic.length() > 0) {
      // Re-subscribe to command topic after reconnect
      mqttClient.subscribe(commandTopic.c_str());
    }
  } else {
    Serial.print(" failed (rc=");
    Serial.print(mqttClient.state());
    Serial.println("). Will retry in 5s.");
  }
}

// ── Sensor Reading Functions ────────────────────────────────────────────────
void readTemperature() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C && t > TEMP_SAFETY_MIN && t < TEMP_SAFETY_MAX) {
    waterTemp = t;
  }
  // Emergency shutoff
  if (waterTemp >= TEMP_SAFETY_MAX) {
    heaterOn = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("[SAFETY] Emergency shutoff! Temp too high.");
  }
}

void readDHT22() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();  // Celsius
  // DHT22 returns NaN on read failure
  if (!isnan(h)) humidity = h;
  if (!isnan(t)) ambientTemp = t;
}

void calculateFlowRate() {
  if (millis() - lastFlowCalc >= FLOW_CALC_INTERVAL_MS) {
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();

    // Flow rate in L/min
    flowRate = (count / FLOW_CALIBRATION_FACTOR);
    lastFlowCalc = millis();
  }
}

void readCurrent() {
  int rawADC = analogRead(ACS712_PIN);
  float voltage = rawADC * ADC_TO_VOLTAGE;
  // Current = (voltage - zero point voltage) / sensitivity
  float zeroVoltage = ACS712_ZERO_POINT * ADC_TO_VOLTAGE;
  currentAmps = abs((voltage - zeroVoltage) / ACS712_SENSITIVITY);
}

void readWaterLevel() {
  // Two XKC-Y25 sensors: one near top (HIGH), one near bottom (LOW)
  // XKC-Y25: HIGH when liquid is detected at that level
  waterLevelHigh = (digitalRead(WATER_LEVEL_HIGH_PIN) == HIGH);
  waterLevelLow  = (digitalRead(WATER_LEVEL_LOW_PIN) == HIGH);

  // Determine water level string:
  //   Both detect water → HIGH
  //   Only LOW detects  → MEDIUM (water is above bottom but below top)
  //   Neither detects   → LOW (tank is nearly empty)
  if (waterLevelHigh && waterLevelLow) {
    waterLevelStr = "HIGH";
  } else if (waterLevelLow) {
    waterLevelStr = "MEDIUM";
  } else {
    waterLevelStr = "LOW";
  }
}

// ── Print Sensor Readings to Console ────────────────────────────────────────
void printSensorReadings() {
  if (millis() - lastSensorPrint < SENSOR_PRINT_INTERVAL_MS) return;
  lastSensorPrint = millis();

  Serial.println("\n--- Sensor Readings ---");
  Serial.print("Water Temp:   "); Serial.print(waterTemp, 1);   Serial.println(" C");
  Serial.print("Ambient Temp: "); Serial.print(ambientTemp, 1); Serial.println(" C");
  Serial.print("Humidity:     "); Serial.print(humidity, 1);     Serial.println(" %");
  Serial.print("Flow Rate:    "); Serial.print(flowRate, 1);     Serial.println(" L/min");
  Serial.print("Current:      "); Serial.print(currentAmps, 2);  Serial.println(" A");
  Serial.print("Water Level:  "); Serial.println(waterLevelStr);
  Serial.print("  (High pin="); Serial.print(waterLevelHigh ? "YES" : "NO");
  Serial.print(", Low pin=");  Serial.print(waterLevelLow ? "YES" : "NO");
  Serial.println(")");
  Serial.print("Heater:       "); Serial.println(heaterOn ? "ON" : "OFF");
  Serial.print("Phase:        "); Serial.println(currentPhase == PHASE_INIT ? "INIT" : "DATA");
  Serial.println("-----------------------");
}

// ── Publish Telemetry ───────────────────────────────────────────────────────
void publishData() {
  if (currentPhase != PHASE_DATA) return;
  if (millis() - lastPublish < PUBLISH_INTERVAL_MS) return;
  lastPublish = millis();

  StaticJsonDocument<384> doc;
  doc["geyser_id"] = assignedGeyserID;
  doc["water_temp"] = round(waterTemp * 10) / 10.0;
  doc["ambient_temp"] = round(ambientTemp * 10) / 10.0;
  doc["humidity"] = round(humidity * 10) / 10.0;
  doc["flow_rate"] = round(flowRate * 10) / 10.0;
  doc["current_amps"] = round(currentAmps * 100) / 100.0;
  doc["water_level"] = waterLevelStr;
  doc["is_heating"] = heaterOn ? 1 : 0;

  // Add temporal context for the ML model
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    doc["hour"] = timeinfo.tm_hour;
    doc["minute"] = timeinfo.tm_min;
    doc["month"] = timeinfo.tm_mon + 1;
    doc["day_of_week"] = timeinfo.tm_wday;
  }

  char buffer[384];
  serializeJson(doc, buffer);

  if (mqttClient.publish(dataTopic.c_str(), buffer)) {
    Serial.print("[DATA] Published: T=");
    Serial.print(waterTemp, 1);
    Serial.print("C, Amb=");
    Serial.print(ambientTemp, 1);
    Serial.print("C, H=");
    Serial.print(humidity, 1);
    Serial.print("%, Flow=");
    Serial.print(flowRate, 1);
    Serial.print(", Level=");
    Serial.println(waterLevelStr);
  }
}

// ── Init Phase: Send Registration ───────────────────────────────────────────
void sendInitRequest() {
  if (initSent && (millis() - initSentTime < 10000)) return; // Wait 10s between retries

  StaticJsonDocument<256> doc;
  doc["mac_address"] = deviceMAC_colon;

  JsonObject info = doc.createNestedObject("info");
  info["chip_model"] = ESP.getChipModel();
  info["chip_revision"] = ESP.getChipRevision();
  info["cores"] = ESP.getChipCores();
  info["firmware_version"] = "1.1.0";

  char buffer[256];
  serializeJson(doc, buffer);

  String initTopic = String(BASE_TOPIC) + "/init";
  mqttClient.publish(initTopic.c_str(), buffer);

  Serial.println("[INIT] Registration sent. Waiting for config...");
  initSent = true;
  initSentTime = millis();
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);  // Give Serial Monitor time to connect
  Serial.println("\n=== Context-Aware Smart Geyser ===");
  Serial.println("Firmware v1.1.0 (No OLED, DHT22 + Dual Water Level)");

  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Start with heater OFF
  pinMode(WATER_LEVEL_HIGH_PIN, INPUT);
  pinMode(WATER_LEVEL_LOW_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  // Attach flow sensor interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseISR, RISING);

  // Initialize temperature sensor (DS18B20)
  tempSensor.begin();
  Serial.println("[OK] DS18B20 initialized.");

  // Initialize DHT22
  dht.begin();
  Serial.println("[OK] DHT22 initialized.");

  Serial.println("[OK] Water level sensors (HIGH pin=" + String(WATER_LEVEL_HIGH_PIN) +
                 ", LOW pin=" + String(WATER_LEVEL_LOW_PIN) + ")");
  Serial.println("[OK] Flow sensor on pin " + String(FLOW_SENSOR_PIN));
  Serial.println("[OK] Current sensor on pin " + String(ACS712_PIN));
  Serial.println("[OK] Relay on pin " + String(RELAY_PIN));

  // Connect WiFi
  connectWiFi();

  // Configure MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  // Sync time via NTP (for temporal features)
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST = UTC+5:30 = 19800s
  Serial.println("[OK] NTP time sync initiated.");
  Serial.println("=====================================\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // ── WiFi check ──────────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    serverConnected = false;
    // MANUAL MODE: Relay stays in whatever state it's in
    printSensorReadings();
    delay(1000);
    connectWiFi(); // Try to reconnect
    return;
  }

  // ── MQTT check ──────────────────────────────────────────────────────────
  if (!mqttClient.connected()) {
    serverConnected = false;
    connectMQTT();
    delay(5000);
    return;
  }
  mqttClient.loop();

  // ── Read sensors (always, regardless of phase) ──────────────────────────
  readTemperature();
  readDHT22();
  calculateFlowRate();
  readCurrent();
  readWaterLevel();

  // ── Phase-specific logic ────────────────────────────────────────────────
  switch (currentPhase) {
    case PHASE_INIT:
      sendInitRequest();
      break;

    case PHASE_DATA:
      publishData();
      break;
  }

  // ── Print to console ──────────────────────────────────────────────────
  printSensorReadings();
}
