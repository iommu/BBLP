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

class MUXOLED {
public:
  MUXOLED();
  ~MUXOLED();
  void draw(uint8_t sel);
  void updateWave(uint8_t sel, Vector<bool> wave);

private:
  void selOLED(uint8_t sel);
  void updateFrame(uint8_t sel);

  Adafruit_SSD1306 display[8];
  Vector<bool> waves[8];
};

class MUXPins {
public:
  MUXPins();
  void writePins(bool pins[4]);
  bool *readPins();

private:
  PCF8574 pcf;
};

class IOInterface : public MUXOLED, public MUXPins {
public:
private:
};

class RGBLED {
public:
  RGBLED(uint8_t r, uint8_t g, uint8_t b);
  void setRGB(uint8_t r, uint8_t g, uint8_t b);

private:
};

class NFC {
public:
  NFC();

  uint32_t getID(); // Gets transmitted ID from NFC
private:
  Adafruit_PN532 nfc;
};

class Interface {
public:
  Interface();

private:
  void updateDisplay(uint8_t question_num);

  RGBLED ind_led;
  NetworkHandler network;
  DynamicJsonDocument j_questions;
  RGBLED rgb_led;
  ESP32Encoder encoder_q; // encoder_t;
  Adafruit_SSD1306 oled;
  //
  // void IRAM_ATTR updateTime(); // Updates small clock top right
  // hw_timer_t * timer = NULL;
  int8_t answer_time;
  uint8_t num_questions;

  //
  uint32_t student_id;
};