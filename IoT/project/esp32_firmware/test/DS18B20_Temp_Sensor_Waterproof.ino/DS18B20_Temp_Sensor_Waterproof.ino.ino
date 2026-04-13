#include <OneWire.h>
#include <DallasTemperature.h>

// Using GPIO 4 on ESP32
#define SENSOR_PIN  14 

OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200); // ESP32 standard baud rate
  sensors.begin();
  
  Serial.println("--- DS18B20 Diagnostic Test ---");
  
  // Check if a sensor is actually connected
  int deviceCount = sensors.getDeviceCount();
  if (deviceCount == 0) {
    Serial.println("ERROR: No sensor detected! Check your wiring and resistor.");
  } else {
    Serial.print("SUCCESS: Found ");
    Serial.print(deviceCount);
    Serial.println(" sensor(s).");
  }
}

void loop() {
  if (sensors.getDeviceCount() > 0) {
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); 
    
    float tempC = sensors.getTempCByIndex(0);

    if (tempC != DEVICE_DISCONNECTED_C) {
      Serial.print("Temperature: ");
      Serial.print(tempC);
      Serial.println("°C");
    } else {
      Serial.println("Error: Could not read temperature data.");
    }
  } else {
    Serial.println("No sensor detected. Check connections.");
  }
  
  delay(2000); 
}