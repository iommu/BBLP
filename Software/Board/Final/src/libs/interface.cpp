#include "interface.hpp"

MUXOLED::MUXOLED() {
  Serial.println("Initializing MUX OLEDs");
  for (uint8_t index = 0; index < 8; index++) {
    display[index] =
        Adafruit_SSD1306(128 /*w*/, 64 /*h*/, &Wire, -1, 800000, 400000);
  }
}

void MUXOLED::selOLED(uint8_t sel) {
  Wire.beginTransmission(0x70 /*TCA chip addr*/);
  Wire.write(1 << sel);
  Wire.endTransmission();
}

// RGBLED

RGBLED::RGBLED() {
  pinMode(12, OUTPUT); // R
  ledcAttachPin(12, 0);
  ledcSetup(0, 4000, 8);
  pinMode(13, OUTPUT); // G
  ledcAttachPin(13, 1);
  ledcSetup(0, 4000, 8);
  pinMode(14, OUTPUT); // B
  ledcAttachPin(14, 2);
  ledcSetup(0, 4000, 8);
}

RGBLED::~RGBLED() {}

void RGBLED::setRGB(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(0, 256 - r); // inverted because led is common anode
  ledcWrite(1, 256 - g); // Note: 256 not 255 as 256 is needed to turn PWM
  ledcWrite(2, 256 - b); // fully on and hence fully off on a common anode RGB
}

Interface::Interface()
    : display(128 /*w*/, 32 /*h*/, &Wire1, -1, 400000, 400000) {}

// NFC

NFC::NFC() : nfc(5 /*CS*/) {
  nfc.begin(); // Start NFC reader

  if (!nfc.getFirmwareVersion()) { // Check we have a PN532 reader
    Serial.print("Could not find PN532 board!");
    while (1)
      ; // Wait forever
  }

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

NFC::~NFC() {}

uint32_t NFC::getID() {
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of UID (4/7 depending on ISO14443A card type)
  bool success =
      nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) { // Can't actually communicate with Android :<
    return 5386949;
  }
}