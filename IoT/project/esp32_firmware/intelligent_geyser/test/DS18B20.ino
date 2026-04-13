#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void) {
  // Start serial communication for debugging
  Serial.begin(9600);
  Serial.println("DS18B20 Temperature Sensor Test");

  // Start up the library
  sensors.begin();
}

void loop(void) { 
  // Call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); 
  Serial.println("DONE");

  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.print(tempC);
    Serial.println("°C");
  } else {
    Serial.println("Error: Could not read temperature data");
  }
  
  delay(2000);
}