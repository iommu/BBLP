#include <Arduino.h>
#include <Wire.h>

#include "libs/interface.hpp"

void setup() {
  Serial.begin(115200); // Setup Serial for general logging
  Wire.begin();   // Setup wire 0 for MUX OLEDs
  Wire1.begin(16, 17, 100000);  // Setup wire 1 for Primary OLED and Pin Mux
  Serial.print("1");
  Interface(); // Start primary interface
}

void loop() {}