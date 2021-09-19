#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_PN532.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Vector.h>

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
private:
};

class IOInterface : public MUXOLED, public MUXPins {
public:
private:
};

class RGBLED {
public:
  RGBLED();
  ~RGBLED();
  void setRGB(uint8_t r, uint8_t g, uint8_t b);

private:
};

class Encoder {
public:
  Encoder();
  ~Encoder();

  void setCallbackTurn();
  void setCallbackSwitch();

private:
  int rotation = 0;

  void (*callback_u)(void); /* rotation update callback */
  void (*callback_b)(void); /* button press callback */
};

class NFC {
public:
  NFC();
  ~NFC();

  uint32_t getID(); // Gets transmitted ID from NFC
private:
  Adafruit_PN532 nfc;
};

class Interface {
public:
  Interface();
  ~Interface();

private:
  DynamicJsonDocument j_questions;
  RGBLED rgb_led;
  Encoder encoders[2];
  Adafruit_SSD1306 oled;
};