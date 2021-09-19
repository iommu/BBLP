#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200); // Setup Serial for general logging
  Wire.begin(); // Setup wire 0 for MUX OLEDs 
  Wire1.begin(); // Setup wire 1 for Primary OLED and Pin Mux
}

void loop() {
  // put your main code here, to run repeatedly:
}