#include "modules.hpp"

// MUXPins : Octa IO expander (4 Input / 4 Output)

MUXPins::MUXPins() : pcf(0x20) /* set I2C addr */ {
  pcf.begin(16, 17); // Start I2C conn
}

void MUXPins::writePins(State pins[4]) {
  // Make sure not to write floating
  for (uint8_t index = 0; index < 4; index++) {
    if (pins[index] == FLOATING) {
      pins[index] = FALSE;
    }
  }
  // Write pins
  pcf.write(mix[0], pins[0]);
  pcf.write(mix[1], pins[1]);
  pcf.write(mix[2], pins[2]);
  pcf.write(mix[3], pins[3]);
}

void MUXPins::readPins(State pins[4]) {
  // Note : PCF is very simple and uses a simple 8 bit register to read/write
  // values and hence does NOT have the ability to return a value determining if
  // the pin is floating. So instead what we do is toggle the pin and read
  // again, if the pin value matches then the output is being held high/low, if
  // it does not then the pin is not floating

  uint8_t read1 = pcf.read8(); // Read pass 1
  uint8_t mask = (1 << mix[4]) | (1 << mix[5]) | (1 << mix[6]) |
                 (1 << mix[7]); // Mask of all outputs
  pcf.toggleMask(mask);         // Toggle output pins
  uint8_t read2 = pcf.read8();  // Read pass 2

  Serial.println("reading");
  Serial.print(read1, BIN);
  Serial.print(" : ");
  Serial.print(read2, BIN);

  // Assign pins using conditional
  pins[0] = (read1 >> mix[4] & 1) == (read2 >> mix[4] & 1)
                ? (State)(read1 >> mix[4] & 1)
                : FLOATING;
  pins[1] = (read1 >> mix[5] & 1) == (read2 >> mix[5] & 1)
                ? (State)(read1 >> mix[5] & 1)
                : FLOATING;
  Serial.println(pins[1]);
  pins[2] = (read1 >> mix[6] & 1) == (read2 >> mix[6] & 1)
                ? (State)(read1 >> mix[6] & 1)
                : FLOATING;
  pins[3] = (read1 >> mix[7] & 1) == (read2 >> mix[7] & 1)
                ? (State)(read1 >> mix[7] & 1)
                : FLOATING;
}

// RGBLED

RGBLED::RGBLED(uint8_t r, uint8_t g, uint8_t b) {
  pinMode(12, OUTPUT); // R
  ledcAttachPin(12, 0);
  ledcSetup(0, 4000, 8);
  pinMode(13, OUTPUT); // G
  ledcAttachPin(13, 1);
  ledcSetup(0, 4000, 8);
  pinMode(14, OUTPUT); // B
  ledcAttachPin(14, 2);
  ledcSetup(0, 4000, 8);
  // Set inital led
  setRGB(r, g, b);
}

void RGBLED::setRGB(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(0, 256 - r); // inverted because led is common anode
  ledcWrite(1, 256 - g); // Note: 256 not 255 as 256 is needed to turn PWM
  ledcWrite(2, 256 - b); // fully on and hence fully off on a common anode RGB
}

// NFC

// NOTE : In the adafruit PN532 lib change line 152 1000000 to 100000 as ESP32
// can't handle speed?
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

uint32_t NFC::getID() {
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of UID (4/7 depending on ISO14443A card type)
  bool success =
      nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) { // Can't actually communicate with Android :<
    return 5386949;
  }
  return 0;
}