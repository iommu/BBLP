#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_PN532.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Encoder.h>
#include <PCF8574.h>
#include <Vector.h>

#include "network.hpp"

// MUX Interface vars

#define PIXELS_PER_BIT 60 // How many pixels on a screen we use for one bit
#define BITS_PER_SECOND 1 // How many new bits we display on a screen per second
#define MAX_BITS 20       // Max number of bits per wave
// MUXOLED : Octa OLED interface

class MUXOLED {
public:
  MUXOLED();

  void draw(uint8_t sel);
  void draw8(int shift);
private:
  void selOLED(uint8_t sel);

  unsigned long time_start; // Time we started this display
  uint32_t pixel_shift;     // how many pixels we don't draw

  bool storage_array[8][MAX_BITS];
  Vector<bool> waves_exp[8]; // Expected waves

  Adafruit_SSD1306 display[8];
};

// MUXPins : Octa IO expander (4 Input / 4 Output)

class MUXPins {
public:
  MUXPins();

  void writePins(bool pins[4]);
  bool *readPins();

private:
  Vector<bool> waves_rec[8]; // Recived waves

  PCF8574 pcf;
};

// IOInterface : MUX handler

class IOInterface : public MUXOLED, public MUXPins {
public:
  IOInterface();

private:
  ESP32Encoder encoder_t;

};

// RGBLED : RGB led handler

class RGBLED {
public:
  RGBLED(uint8_t r, uint8_t g, uint8_t b);
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

// Interface : Primary interface code

class Interface {
public:
  Interface();

private:
  void updateDisplay(uint8_t question_num);

  TaskHandle_t MUXTask;
  RGBLED ind_led;
  NetworkHandler network;
  DynamicJsonDocument j_questions;
  RGBLED rgb_led;
  ESP32Encoder encoder_q;
  Adafruit_SSD1306 oled;
  //
  // void IRAM_ATTR updateTime(); // Updates small clock top right
  // hw_timer_t * timer = NULL;
  int8_t answer_time;
  uint8_t num_questions;

  //
  uint32_t student_id;
};