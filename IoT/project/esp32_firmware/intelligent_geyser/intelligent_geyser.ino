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
// Display: SH1106 128x64 OLED via U8g2 (I2C on GPIO 32/33)
//
// Target Board: ESP32-D0WD-V3 (38-pin DevKit)
// ============================================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"

// ── Global Objects ──────────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient mqttClient(espClient);

OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);

DHT dht(DHT22_PIN, DHT22);

// SH1106 OLED display (matching working test code)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

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
bool waterLevelHigh = false;
bool waterLevelLow = false;
String waterLevelStr = "LOW";
bool heaterOn = false;
bool serverConnected = false;
bool oledAvailable = false;

// Flow sensor interrupt
volatile unsigned long pulseCount = 0;
unsigned long lastFlowCalc = 0;

// Timing
unsigned long lastPublish = 0;
unsigned long lastSensorPrint = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long initSentTime = 0;
bool initSent = false;

// ── Interrupt Service Routine (Flow Sensor) ─────────────────────────────────
void IRAM_ATTR flowPulseISR() {
  pulseCount++;
}

// ── WiFi Connection ─────────────────────────────────────────────────────────
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
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
      String configTopic = String(BASE_TOPIC) + "/config/" + deviceMAC_colon;
      mqttClient.subscribe(configTopic.c_str());
      Serial.println("Subscribed to: " + configTopic);
    } else if (currentPhase == PHASE_DATA && commandTopic.length() > 0) {
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
  if (waterTemp >= TEMP_SAFETY_MAX) {
    heaterOn = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("[SAFETY] Emergency shutoff! Temp too high.");
  }
}

void readDHT22() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h)) humidity = h;
  if (!isnan(t)) ambientTemp = t;
}

void calculateFlowRate() {
  if (millis() - lastFlowCalc >= FLOW_CALC_INTERVAL_MS) {
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();
    flowRate = (count / FLOW_CALIBRATION_FACTOR);
    lastFlowCalc = millis();
  }
}

void readCurrent() {
  int minADC = 4095;
  int maxADC = 0;
  
  // Sample the AC wave for 50ms (covers ~2.5 full 50Hz cycles)
  uint32_t start_time = millis();
  while((millis() - start_time) < 50) { 
    int rawADC = analogRead(ACS712_PIN);
    if (rawADC < minADC) minADC = rawADC;
    if (rawADC > maxADC) maxADC = rawADC;
  }
  
  // Calculate Peak-to-Peak ADC
  int peakToPeakADC = maxADC - minADC;

  // Convert to Peak Voltage
  float voltagePeak = (peakToPeakADC / 2.0) * ADC_TO_VOLTAGE;

  // Calculate RMS Voltage (Vrms = Vpeak * 0.707)
  float voltageRMS = voltagePeak * 0.707;

  // Calculate RMS Current
  currentAmps = voltageRMS / ACS712_SENSITIVITY;

  // ── Noise Floor Cutoff ──
  // The ACS712 on a breadboard picks up heavy ambient electromagnetic noise,
  // causing it to read 0.5A - 0.75A even when the load is completely disconnected.
  // Since a real geyser draws massive current (9 Amps to 15 Amps), we can safely
  // snap any reading below 0.85A down to 0.00A to eliminate "phantom" readings.
  if(currentAmps < 0.85) {
    currentAmps = 0.0;
  } else {
    // Optional: Subtract the base noise floor so the reported current is more accurate
    // currentAmps = currentAmps - 0.71; 
  }
}

void readWaterLevel() {
  waterLevelHigh = (digitalRead(WATER_LEVEL_HIGH_PIN) == HIGH);
  waterLevelLow  = (digitalRead(WATER_LEVEL_LOW_PIN) == HIGH);

  if (waterLevelHigh && waterLevelLow) {
    waterLevelStr = "HIGH";
  } else if (waterLevelLow) {
    waterLevelStr = "MEDIUM";
  } else {
    waterLevelStr = "LOW";
  }
}

// ── OLED Display Update ─────────────────────────────────────────────────────
void updateDisplay() {
  if (!oledAvailable) return;
  if (millis() - lastDisplayUpdate < DISPLAY_INTERVAL_MS) return;
  lastDisplayUpdate = millis();

  char buf[32];  // Temp buffer for formatting strings

  u8g2.clearBuffer();

  // ── Row 1: Status bar (bold) ──────────────────────────────────────────
  u8g2.setFont(u8g2_font_6x10_tf);
  if (currentPhase == PHASE_INIT) {
    u8g2.drawStr(0, 10, "INIT... Waiting");
  } else {
    snprintf(buf, sizeof(buf), "Geyser #%d  %s", assignedGeyserID,
             serverConnected ? "[OK]" : "[OFF]");
    u8g2.drawStr(0, 10, buf);
  }

  // Horizontal separator
  u8g2.drawHLine(0, 12, 128);

  // ── Row 2: Water Temp & Ambient Temp ──────────────────────────────────
  u8g2.setFont(u8g2_font_5x8_tf);

  snprintf(buf, sizeof(buf), "Wtr:%s%dC", waterTemp < 100 ? " " : "", (int)waterTemp);
  u8g2.drawStr(0, 23, buf);
  snprintf(buf, sizeof(buf), "Amb:%s%dC", ambientTemp < 100 ? " " : "", (int)ambientTemp);
  u8g2.drawStr(68, 23, buf);

  // ── Row 3: Humidity & Flow ────────────────────────────────────────────
  snprintf(buf, sizeof(buf), "Hum: %d%%", (int)humidity);
  u8g2.drawStr(0, 33, buf);
  // Format flow with 1 decimal
  int flowWhole = (int)flowRate;
  int flowFrac = (int)((flowRate - flowWhole) * 10);
  snprintf(buf, sizeof(buf), "Flw: %d.%dL/m", flowWhole, flowFrac);
  u8g2.drawStr(68, 33, buf);

  // ── Row 4: Current & Water Level ──────────────────────────────────────
  int ampsWhole = (int)currentAmps;
  int ampsFrac = (int)((currentAmps - ampsWhole) * 100);
  snprintf(buf, sizeof(buf), "Cur: %d.%02dA", ampsWhole, ampsFrac);
  u8g2.drawStr(0, 43, buf);
  snprintf(buf, sizeof(buf), "Lvl: %s", waterLevelStr.c_str());
  u8g2.drawStr(68, 43, buf);

  // Horizontal separator
  u8g2.drawHLine(0, 46, 128);

  // ── Row 5: Heater status (prominent) ──────────────────────────────────
  u8g2.setFont(u8g2_font_6x10_tf);
  snprintf(buf, sizeof(buf), "HEATER: %s", heaterOn ? "ON" : "OFF");
  u8g2.drawStr(0, 58, buf);

  // WiFi indicator on the right
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(90, 58, WiFi.status() == WL_CONNECTED ? "WiFi:OK" : "WiFi:--");

  u8g2.sendBuffer();
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
  if (initSent && (millis() - initSentTime < 10000)) return;

  StaticJsonDocument<256> doc;
  doc["mac_address"] = deviceMAC_colon;

  JsonObject info = doc.createNestedObject("info");
  info["chip_model"] = ESP.getChipModel();
  info["chip_revision"] = ESP.getChipRevision();
  info["cores"] = ESP.getChipCores();
  info["firmware_version"] = "1.2.0";

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
  delay(1000);
  Serial.println("\n=== Context-Aware Smart Geyser ===");
  Serial.println("Firmware v1.2.0 (SH1106 OLED + DHT22 + Dual Water Level)");

  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(WATER_LEVEL_HIGH_PIN, INPUT);
  pinMode(WATER_LEVEL_LOW_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);

  // Attach flow sensor interrupt
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseISR, RISING);

  // Initialize DS18B20
  tempSensor.begin();
  Serial.println("[OK] DS18B20 initialized.");

  // Initialize DHT22
  dht.begin();
  Serial.println("[OK] DHT22 initialized.");

  // Initialize OLED (SH1106 via U8g2, matching test code)
  delay(250);  // Give display time to power up
  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  oledAvailable = true;
  Serial.println("[OK] SH1106 OLED initialized (SDA=" + String(OLED_SDA) +
                 ", SCL=" + String(OLED_SCL) + ")");

  // Show boot screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(10, 20, "Smart Geyser");
  u8g2.drawStr(10, 35, "v1.2.0");
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(10, 50, "Booting...");
  u8g2.sendBuffer();

  Serial.println("[OK] Sensors: Water Level HIGH=" + String(WATER_LEVEL_HIGH_PIN) +
                 ", LOW=" + String(WATER_LEVEL_LOW_PIN));
  Serial.println("[OK] Flow sensor pin " + String(FLOW_SENSOR_PIN));
  Serial.println("[OK] Current sensor pin " + String(ACS712_PIN));
  Serial.println("[OK] Relay pin " + String(RELAY_PIN));

  // Connect WiFi
  connectWiFi();

  // Configure MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);

  // Sync time via NTP
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("[OK] NTP time sync initiated.");
  Serial.println("=====================================\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // ── ALWAYS read sensors first, regardless of connectivity ───────────────
  readTemperature();
  readDHT22();
  calculateFlowRate();
  readCurrent();
  readWaterLevel();

  // ── ALWAYS update display and console ───────────────────────────────────
  updateDisplay();
  printSensorReadings();

  // ── WiFi check ──────────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    serverConnected = false;
    delay(1000);
    connectWiFi();
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

  // ── Phase-specific logic ────────────────────────────────────────────────
  switch (currentPhase) {
    case PHASE_INIT:
      sendInitRequest();
      break;
    case PHASE_DATA:
      publishData();
      break;
  }
}

