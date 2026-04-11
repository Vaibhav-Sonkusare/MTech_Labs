// ============================================================================
// config.h — Configuration for Context-Aware Smart Geyser (ESP32)
// ============================================================================
#ifndef CONFIG_H
#define CONFIG_H

// ── WiFi Credentials ────────────────────────────────────────────────────────
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ── MQTT Broker ─────────────────────────────────────────────────────────────
#define MQTT_BROKER     "broker.hivemq.com"
#define MQTT_PORT       1883
#define BASE_TOPIC      "smart_geyser"

// ── Sensor Pins (38-pin ESP32-D0WD-V3 DevKit) ──────────────────────────────
#define DS18B20_PIN     4       // OneWire data pin for temperature sensor
#define FLOW_SENSOR_PIN 27      // Interrupt-capable pin for YF-S201
#define ACS712_PIN      34      // ADC pin for current sensor (input only)
#define WATER_LEVEL_PIN 25      // Digital input for XKC-Y25

// ── Actuator Pins ───────────────────────────────────────────────────────────
#define RELAY_PIN       32      // Relay/SSR control for heating element

// ── Display (I2C OLED SSD1306) ──────────────────────────────────────────────
#define OLED_SDA        21
#define OLED_SCL        22
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1      // No reset pin

// ── Timing ──────────────────────────────────────────────────────────────────
#define PUBLISH_INTERVAL_MS   5000    // Send telemetry every 5 seconds
#define DISPLAY_INTERVAL_MS   1000    // Refresh OLED every 1 second
#define FLOW_CALC_INTERVAL_MS 1000    // Calculate flow rate every 1 second

// ── Flow Sensor Calibration ─────────────────────────────────────────────────
// YF-S201: ~7.5 pulses per litre per minute (from datasheet)
#define FLOW_CALIBRATION_FACTOR  7.5

// ── ACS712 Calibration (5A module) ──────────────────────────────────────────
// At 0A, output is Vcc/2 (~2.5V). Sensitivity: 185mV/A for 5A module.
// ESP32 ADC: 12-bit (0-4095), 0-3.3V range.
#define ACS712_ZERO_POINT   2048     // ADC value at 0 Amps (~1.65V for 3.3V ref)
#define ACS712_SENSITIVITY  0.185    // V/A for 5A module
#define ADC_TO_VOLTAGE      (3.3 / 4095.0)

// ── Safety Thresholds ───────────────────────────────────────────────────────
#define TEMP_SAFETY_MIN  10.0   // Below this, something is wrong with the sensor
#define TEMP_SAFETY_MAX  85.0   // Above this, emergency shutoff

#endif // CONFIG_H
