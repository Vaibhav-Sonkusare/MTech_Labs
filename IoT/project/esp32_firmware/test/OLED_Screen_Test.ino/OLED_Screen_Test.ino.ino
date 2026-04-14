#include <U8g2lib.h>
#include <Wire.h>

// U8g2 Constructor for SSH1106 128x64 I2C
// Rotation, Clock, Data, Reset (U8X8_PIN_NONE if no reset pin)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  // Small delay to ensure the display is powered up before communication starts
  delay(250); 

  // Wire.begin(SDA, SCL);
  Wire.begin(32, 33); 

  u8g2.begin();
}

void loop() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "ESP32 Connected!");
  u8g2.sendBuffer();
  delay(1000);
}