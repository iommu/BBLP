#include <Arduino.h>
#include <Wire.h>

#include "libs/interface.hpp"

void setup() {
  Serial.begin(115200); // Setup Serial for general logging

  Wire.begin(16, 17); // Setup wire 1 for Primary OLED and Pin Mux

  Wire1.begin(21, 22); // Setup wire 0 for MUX OLEDs


  Interface(); // Start primary interface
}

void loop() {}