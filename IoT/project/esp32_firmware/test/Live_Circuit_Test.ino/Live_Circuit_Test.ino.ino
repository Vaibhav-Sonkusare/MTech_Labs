#define RELAY_PIN 23
#define CURRENT_PIN 34

// Calibration values (you may tweak later)
float sensitivity = 0.185;   // For ACS712 5A module (change if needed)
float offsetVoltage = 2.5;   // Default midpoint (no current)

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  // Relay OFF initially (LOW-level trigger modules invert logic)
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("System Initialized...");
}

void loop() {

  Serial.println("Turning RELAY ON...");
  digitalWrite(RELAY_PIN, LOW);  // ON (low trigger)
  delay(2000);

  float current = readCurrent();
  Serial.print("Current (ON): ");
  Serial.print(current);
  Serial.println(" A");

  delay(3000);

  Serial.println("Turning RELAY OFF...");
  digitalWrite(RELAY_PIN, HIGH); // OFF
  delay(2000);

  current = readCurrent();
  Serial.print("Current (OFF): ");
  Serial.print(current);
  Serial.println(" A");

  Serial.println("----------------------");

  delay(5000);
}


// Function to read AC current (simple version)
float readCurrent() {
  int samples = 100;
  float sum = 0;

  for (int i = 0; i < samples; i++) {
    int adcValue = analogRead(CURRENT_PIN);
    float voltage = (adcValue / 4095.0) * 3.3;
    float current = (voltage - offsetVoltage) / sensitivity;
    sum += current * current;
    delay(2);
  }

  float rmsCurrent = sqrt(sum / samples);
  return rmsCurrent;
}