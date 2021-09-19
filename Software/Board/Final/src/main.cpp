#include <Arduino.h>
#include <Wire.h>

#include "libs/interface.hpp"

RGBLED leds = RGBLED();

void setup() {
  Serial.begin(115200); // Setup Serial for general logging
  Wire.begin();         // Setup wire 0 for MUX OLEDs
  Wire1.begin();        // Setup wire 1 for Primary OLED and Pin Mux
}

void loop() {
  leds.setRGB(255, 0, 0);
  delay(2000);

    leds.setRGB(128, 0, 0);
  delay(2000);

      leds.setRGB(0, 0, 0);
  delay(2000);

  leds.setRGB(0, 255, 0);
  delay(2000);

  leds.setRGB(0, 0, 255);
  delay(2000);
}