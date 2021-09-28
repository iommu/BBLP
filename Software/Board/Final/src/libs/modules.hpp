#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_PN532.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Encoder.h>
#include <PCF8574.h>


// MUXPins : Octa IO expander (4 Input / 4 Output)

enum State { // PCF pin states
  FALSE = 0,
  TRUE = 1,
  FLOATING = 2
};

class MUXPins {
public:
  MUXPins();

  void writePins(State pins[4]);
  void readPins(State pins[4], bool floating);

private:
  uint8_t mix[8] = {0, 5, 6, 7, 1, 2, 3, 4}; // Switch Pin names with pcf addr
  PCF8574 pcf;
};

// RGBLED : RGB led handler

class RGBLED {
public:
  RGBLED(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
  void setRGB(uint8_t r, uint8_t g, uint8_t b);

private:
};

// NFC : PN532 handler

class NFC {
public:
  NFC();

  uint32_t getID(); // Gets transmitted ID from NFC
private:
  Adafruit_PN532 nfc;
};
