#include "interface.hpp"

// MUXOLED : Octa OLED interface

MUXOLED::MUXOLED() {
  Serial.println("Initializing MUX OLEDs");
  for (uint8_t index = 0; index < 8; index++) {
    display[index] =
        Adafruit_SSD1306(128 /*w*/, 64 /*h*/, &Wire, -1, 800000, 400000);
    selOLED(index);
    if (!display[index].begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.print(F("SSD1306 allocation failed For OLED : "));
      Serial.println(index);
      for (;;)
        ; // Don't proceed, loop forever
    }
    // Set screen upside down if one of the lucky few
    switch (index) {
    case 4:
    case 5:
    case 6:
    case 7:
      display[index].setRotation(2);
    }

    // Clear OLEDs
    display[index].clearDisplay();
    display[index].display();

    // Set storage array for waves_exp "Vector" clone
    waves_exp[index].setStorage(storage_array[index]);
    waves_exp[index].push_back(0);
    waves_exp[index].push_back(1);
    waves_exp[index].push_back(0);
    waves_exp[index].push_back(1);
    waves_exp[index].push_back(0);
    waves_exp[index].push_back(1);
    waves_exp[index].push_back(0);
    waves_exp[index].push_back(1);
  }
  time_start = millis(); // time drawing started
  pixel_shift = (float)((millis() - time_start) / 1000) * BITS_PER_SECOND *
                PIXELS_PER_BIT;
}

void MUXOLED::draw8(int shift) {
  // capped += , make sure never smaller than 0
  pixel_shift = ((int)((int)pixel_shift + shift) < 0) ? 0 : pixel_shift + shift;

  Serial.println(pixel_shift);
  for (uint8_t index = 0; index < 8; index++) {
    draw(index);
  }
}

void MUXOLED::draw(uint8_t sel) {
  uint start_bit = pixel_shift / PIXELS_PER_BIT;    // First display bit
  uint delta = ceil((float)(128) / PIXELS_PER_BIT); // Number of bits per frame

  // Cap range
  if (waves_exp[sel].size() <= start_bit + delta)
    delta = waves_exp[sel].size() - start_bit;

  // Change active I2C display
  selOLED(sel);
  display[sel].clearDisplay();
  for (uint index = 0; index < delta; index++) {
    uint y_loc = waves_exp[sel][start_bit + index]
                     ? 0
                     : 60; // true = high = 0, false = low = 60

    uint x_loc = -(pixel_shift % PIXELS_PER_BIT) + index * PIXELS_PER_BIT;

    display[sel].drawLine(x_loc, y_loc, x_loc + PIXELS_PER_BIT, y_loc,
                          SSD1306_WHITE); // Draw hor line

    // If next bit is not same as current bit draw next bit
    if (waves_exp[sel][start_bit + index] !=
        waves_exp[sel][start_bit + index + 1]) {
      display[sel].drawLine(x_loc + PIXELS_PER_BIT, 0, x_loc + PIXELS_PER_BIT,
                            60,
                            SSD1306_WHITE); // Draw hor line
    }
  }
  display[sel].display();
}

void MUXOLED::selOLED(uint8_t sel) {
  Wire.beginTransmission(0x70 /*TCA chip addr*/);
  Wire.write(1 << sel);
  Wire.endTransmission();
}

// MUXPins : Octa IO expander (4 Input / 4 Output)

MUXPins::MUXPins() : pcf(0x20) /* set I2C addr */ {
  pcf.begin(16, 17); // Start I2C conn
}

void MUXPins::writePins(bool pins[4]) {
  pcf.write(0, pins[0]);
  pcf.write(1, pins[1]);
  pcf.write(2, pins[2]);
  pcf.write(3, pins[3]);
}

bool *MUXPins::readPins() {
  bool pins[4];
  pins[0] = pcf.read(4);
  pins[1] = pcf.read(5);
  pins[2] = pcf.read(6);
  pins[3] = pcf.read(7);
  return pins;
}

// IOInterface

bool t_state = false;
void tBtnPress() { t_state = true; }

IOInterface::IOInterface() : oleds() {
  encoder_t.attachHalfQuad(32, 34); // Time
  encoder_t.clearCount();           // Clear value

  pinMode(33, INPUT);
  attachInterrupt(33, tBtnPress, FALLING); // Time

  bool auto_shift = 0;
  int count_shift = 0, old_count = 0;
  while (1) {
    if (t_state) {
      auto_shift = !auto_shift;
      t_state = false;
    }

    count_shift = encoder_t.getCount() - old_count;
    old_count = encoder_t.getCount();
    oleds.draw8(count_shift * 20 + auto_shift * 4);
  }
}

void interfaceFunc(void *parameter) { IOInterface interface = IOInterface(); }

// RGBLED

RGBLED::RGBLED(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) {
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

// Interface

bool q_state = false;

void qBtnPress() { q_state = true; }

Interface::Interface()
    : ind_led(), network(), j_questions(2048),
      oled(128 /*w*/, 32 /*h*/, &Wire1, -1) {
  // Setup OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Primary SSD1306 allocation failed");
    for (;;)
      ; // Don't proceed, loop forever
  }

  { // Show welcome screen
    Serial.println("Showing welcome screen");
    oled.clearDisplay();
    oled.invertDisplay(true);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    oled.println(F("Test board"));
    oled.setTextSize(1);
    oled.setCursor(70, 20);
    oled.println("Ver : 0.2");
    oled.display();
    delay(2000);
  }

  { // Connect to wifi and get time
    network.updateTime();
  }

  { // Connect to wifi and pull server json
    oled.clearDisplay();
    Serial.println("Downloading questions from server");
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1.5);
    oled.setCursor(0, 0);
    oled.println(F("Downloading question's"));
    oled.display();
    deserializeJson(j_questions,
                    network.getQuestions()); // Convert network String to JSON
    // num_question = ...
    num_questions = 5;
  }

  { // Getting student ID from NFC
    oled.clearDisplay();
    Serial.println("Waiting for NFC student ID");
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    oled.println(F("Scan ID   Card")); // weird spacing because of text roll
    oled.display();
    NFC nfc = NFC();
    student_id = nfc.getID();
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.println("Student ID");
    oled.println(student_id);
    Serial.print("Got student id : ");
    Serial.print(student_id);
    Serial.println(" : from NFC");
    oled.display();
    delay(3000);
  }

  { // Display basic exam info
    oled.clearDisplay();
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(0, 0);
    oled.print(num_questions);
    oled.println(F(" question"));
    oled.println(F("2 hours"));
    oled.display();
  }

  // { // Setup time draw function
  //   timer = timerBegin(0, 80000000, true); // Setup timer to pulse every
  //   80Mpulses (1s @ 80MHz) timerAttachInterrupt(timer,
  //   std::bind(&Interface::updateTime, this), true); // Attach our update func
  //   timerAlarmWrite(timer, 60, true); // Set timer intrupt every 60 units
  //   (unit = 1s) timerAlarmEnable(timer); // enable timer
  // }

  { // Attach interrupts to rot enc
    // Attach encoders to ESP32encoder class
    encoder_q.attachHalfQuad(25, 26); // Question
    encoder_q.clearCount();           // Clear value

    // Attach button handler
    pinMode(27, INPUT);
    attachInterrupt(27, qBtnPress, FALLING); // Question
  }

  { // Start encoder UI, while loop because OOP callbacks just not worth it in C
    bool init = false;             // checks if user is on launch page
    bool running = false;          // running a question
    unsigned long revert_time = 0; // used to revert to screen after 3 seconds
                                   // of non selected question
    uint8_t view_question = 0,
            sel_question = 0; // current view / current selected
    MUXTask = NULL; // Set task null till it's used
    
    while (1) {
      if (revert_time && revert_time < millis()) {
        revert_time = 0;
        view_question = sel_question;     // reset to reverted
        encoder_q.setCount(sel_question); // Set encoder back to question
        updateDisplay(view_question);     // remove bar / refresh screen
        // Invert screen to show active
        oled.invertDisplay(true);
        oled.display();
      } else if (q_state && init) {
        q_state = false;              // reset button
        running = true;               // we are now running a question
        revert_time = 0;              // reset revert time if selected question
        sel_question = view_question; // we clicked = we want
        encoder_q.setCount(sel_question); // Set encoder back to question
                                          // (incase changed during push)
        updateDisplay(view_question);     // remove bar / refresh screen

        // select the question and start

        // Invert screen to show active
        oled.invertDisplay(true);
        oled.display();
        // Start question
        Serial.println("ehhh");
        if (MUXTask != NULL) {
          vTaskDelete(MUXTask);
        }
                Serial.println("ehhhggghhh");

        xTaskCreatePinnedToCore(
            interfaceFunc,   /* Function to implement the task */
            "mux interface", /* Name of the task */
            10000,           /* Stack size in words */
            NULL,            /* Task input parameter */
            0,               /* Priority of the task */
            &MUXTask,        /* Task handle. */
            0);              /* Core where the task should run */
      } else if (encoder_q.getCount() % num_questions != view_question) {
        if (running) {
          revert_time = millis() + 3000; // revert in 3 seconds past now
        }
        if (!init) {
          init = true;           // We've changed the variable so let us use it
          encoder_q.setCount(0); // reset to 0
        }
        // Wrap if neg
        if (encoder_q.getCount() < 0) {
          encoder_q.setCount(num_questions - 1);
        }
        // Change question
        view_question = encoder_q.getCount() % num_questions;
        updateDisplay(view_question);
      }

      if (revert_time && revert_time >= millis()) { // Draw bar
        uint8_t bar =
            128 - ((revert_time - millis()) * 128 /
                   3000); // convert time left to bar of length of screen
        oled.fillRect(0, 29, bar, 3, SSD1306_WHITE); // draw bar
        oled.display();
      }
      delay(50);
    }
  }
}

void Interface::updateDisplay(uint8_t question_num) {
  oled.clearDisplay();
  oled.invertDisplay(false);
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.print(F("Question "));
  oled.println(question_num + 1);
  oled.setTextSize(1);
  oled.setCursor(0, 20);
  oled.print(F("Of "));
  oled.println(num_questions);
  oled.display();
}

// void IRAM_ATTR Interface::updateTime() {}