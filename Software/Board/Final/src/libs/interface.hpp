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
#define START_OFFSET 64   // Number pixels to offset the middle
#define MAX_BITS 20       // Max number of bits per wave

// MUXPins : Octa IO expander (4 Input / 4 Output)

enum state { // PCF pin states
  FLOATING = -1,
  FALSE = 0,
  TRUE = 1
};

class MUXPins {
public:
  MUXPins();

  void writePins(state pins[4]);
  void readPins(state pins[4]);

private:
  Vector<bool> waves_rec[8]; // Recived waves

  uint8_t mix[8] = {0, 5, 6, 7, 1, 2, 3, 4}; // Switches Pin names with pcf addr

  PCF8574 pcf;
};

// MUXOLED : Octa OLED interface

class MUXOLED : public MUXPins {
public:
  MUXOLED();

  void draw(uint8_t sel, uint start_bit, uint delta);
  void draw8(int shift);

private:
  void selOLED(uint8_t sel);

  uint8_t mix[8] = {3, 2, 1, 0,
                    4, 5, 6, 7}; // Switches OLED names with mux addr

  uint8_t recorded_index = 0;
  uint64_t recorded[4] = {0}; // one bit per PIXELS_PER_BIT/12 pixels

  uint16_t pixel_shift, old_shift; // how many pixels we don't draw

  bool storage_array[8][MAX_BITS] = {{false}};
  Vector<bool> waves_exp[8]; // Expected waves

  Adafruit_SSD1306 display[8];
};

// IOInterface : MUX handler

class IOInterface {
public:
  IOInterface();

private:
  MUXOLED oleds;
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