#include "interface.hpp"

std::mutex i2c; // I2C lock

// MUXPins : Octa IO expander (4 Input / 4 Output)

MUXPins::MUXPins() : pcf(0x20) /* set I2C addr */ {
  pcf.begin(16, 17); // Start I2C conn
}

void MUXPins::writePins(State pins[4]) {
  // Make sure not to write floating
  for (uint8_t index = 0; index < 4; index++) {
    if (pins[index] = FLOATING) {
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

// BItArray : Bit array handling class for large array of

void CrumbArray::setCru(uint16_t index, uint8_t crumb) {
  Serial.print("setting : ");
  Serial.print(index);
  Serial.print(" to ");
  Serial.println(crumb, BIN);

  index *= 2; // double as we're index every second bit
  bits[index / 32] &= ~(0b11 << (index % 32)); // Force crumb to 0
  bits[index / 32] |= crumb << (index % 32);   // Set specific crumb
}

uint8_t CrumbArray::getCru(uint16_t index) {
  Serial.print("getting : ");
  Serial.print(index);
  Serial.print(" : ");
  index *= 2; // double as we're index every second bit
  Serial.println((bits[index / 32] >> (index % 32)) & 0b11);
  Serial.print(index / 32);
  Serial.print(" , ");
  Serial.println(index % 32);
  return (bits[index / 32] >> (index % 32)) &
         0b11; // get bitshifted crumb via masking
}

// IOInterface : Octa OLED interface and pin handler

// Non OOP handler
bool t_state;
void tBtnPress() { t_state = true; }

IOInterface::IOInterface(JsonArray j_output, JsonArray j_input) {
  Serial.println("Initializing MUX OLEDs");
  Wire1.begin(21, 22); // RE-setup for some reason?

  for (uint8_t index = 0; index < 8; index++) {
    // Create a new display object
    i2c.lock();
    display[index] =
        Adafruit_SSD1306(128 /*w*/, 64 /*h*/, &Wire1, -1, 800000, 400000);

    // Select OLED using I2C MUX
    selOLED(index);

    // Initialise display
    if (!display[index].begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.print(F("SSD1306 allocation failed For OLED : "));
      Serial.println(index);
      for (;;)
        ; // Don't proceed, loop forever
    }

    // Set screen upside down if one of the lucky few
    if (index > 3) {
      display[index].setRotation(2);
    }

    // Clear OLEDs
    display[index].clearDisplay();
    display[index].display();
    i2c.unlock();

    // Set storage array for waves_exp "Vector" clone
    waves_exp[index].setStorage(storage_array[index]);
    // waves_exp[index].push_back(0);
    // waves_exp[index].push_back(1);
    // waves_exp[index].push_back(0);
    // waves_exp[index].push_back(1);
    // waves_exp[index].push_back(0);
    // waves_exp[index].push_back(1);
    // waves_exp[index].push_back(0);
    // waves_exp[index].push_back(1);
  }

  // Attach all interrupts
  encoder_t.attachHalfQuad(32, 34); // Time

  pinMode(33, INPUT);
  attachInterrupt(33, tBtnPress, FALLING); // Time

  max_exp_bits = 0; // Reset
  Serial.println("setting waves");
  Serial.println(j_output.size());
  Serial.println(j_input.size());
  // For ouput waves
  for (uint8_t wave_index = 0; wave_index < j_output.size(); wave_index++) {
    waves_exp[wave_index].clear();                    // Clear all waves
    String ouput = j_output[wave_index].as<String>(); // Type cast
    Serial.print("ouput : ");
    // Inter over ouput waves
    for (char bit : ouput) {
      // Push back bit
      if (bit == '0') {
        waves_exp[wave_index].push_back(0);
        Serial.print(0);
      } else { // 1 (no other option)
        Serial.print(1);
        waves_exp[wave_index].push_back(1);
      }
    }
    Serial.println();
    max_exp_bits = (max_exp_bits < ouput.length())
                       ? max_exp_bits = ouput.length()
                       : max_exp_bits; // Update max_exp_bits if new max found
  }

  // For input waves
  for (uint8_t wave_index = 4; wave_index < j_input.size() + 4; wave_index++) {
    waves_exp[wave_index].clear();                       // Clear all waves
    String input = j_input[wave_index - 4].as<String>(); // Type cast
    Serial.print("input : ");
    Serial.println(input);
    // Inter over input waves
    for (char bit : input) {
      // Push back bit
      if (bit == '0') {
        waves_exp[wave_index].push_back(0);
      } else { // 1 (no other option)
        waves_exp[wave_index].push_back(1);
      }
    }

    max_exp_bits = (max_exp_bits < input.length())
                       ? max_exp_bits = input.length()
                       : max_exp_bits; // Update max_exp_bits if new max found
  }

  // Reset interrupt values
  encoder_t.clearCount(); // Clear value
  t_state = false;

  // Reset display values
  pixel_shift = 0;
  old_shift = 0;

  // Set main interface values
  bool auto_shift = 0;
  int new_count = 0, old_count = 0;

  // Initial draw
  draw8(0);

  // Main interface loop
  while (1) {
    // If button press
    if (t_state) {
      auto_shift = !auto_shift;
      t_state = false;
    }

    new_count = encoder_t.getCount();
    if (auto_shift || (old_count != new_count)) {
      draw8((new_count - old_count) * 20 +
            auto_shift * 5); // Encoder differential * 20 + auto * 5
      old_count = new_count;
    }
  }
}

void IOInterface::draw8(int shift) {
  // Capped at 0 and bits * PIXEL_PER_BIT + (half of screen);
  if (pixel_shift + shift < 0) {
    pixel_shift = 0;
  } else if (pixel_shift + shift > waves_exp[0].size() * PIXELS_PER_BIT) {
    pixel_shift = waves_exp[0].size() * PIXELS_PER_BIT;
  } else {
    pixel_shift += shift;
  }

  Serial.print("pixel shift");
  Serial.println(pixel_shift);

  uint start_bit = pixel_shift / PIXELS_PER_BIT; // First display bit
  Serial.println(start_bit);
  int delta = ceil((float)(128) / PIXELS_PER_BIT); // Number of bits per frame
  Serial.println(delta);

  { // Update IO for all filler positions
    // Set current
    State pins[] = {
        (State)waves_exp[0][start_bit], (State)waves_exp[1][start_bit],
        (State)waves_exp[2][start_bit], (State)waves_exp[3][start_bit]};
    writePins(pins);
    readPins(pins);

    uint8_t write_index = pixel_shift / 20;
    recorded[0].setCru(write_index, pins[0]);
    Serial.println(pins[1]);
    recorded[1].setCru(write_index, pins[1]);
    recorded[2].setCru(write_index, pins[2]);
    recorded[3].setCru(write_index, pins[3]);
  }

  // Draw all screens

  for (uint8_t index = 0; index < 8; index++) {
    draw(index, start_bit, delta);
  }

  // Update old_shift
  old_shift = pixel_shift;
}

void IOInterface::draw(uint8_t sel, uint start_bit, int delta) {
  // Don't draw if out of range
  if (waves_exp[sel].size() == 0) {
    Serial.print("not drawing ");
    Serial.println(sel);
    selOLED(sel);
    display[sel].clearDisplay();
    display[sel].display();
    return;
  }
  Serial.print("drawing ");
  Serial.print(sel);
  Serial.print(" : ");
  for (bool test : waves_exp[sel])
    Serial.print(test);
  Serial.println("ehh");
  // Cap range
  if (waves_exp[sel].size() + 1 <= start_bit + delta)
    delta = waves_exp[sel].size() - start_bit + 1;

  // Change active I2C display
  selOLED(sel);
  display[sel].clearDisplay();

  // Draw primary lines
  for (uint8_t index = 0; index < delta; index++) {
    // If we're trying to access negative bit
    if (start_bit == 0 && index == 0)
      continue;
    Serial.println(waves_exp[sel].size());
    Serial.println(start_bit);
    Serial.println(index);
    Serial.print(index);
    Serial.println(pixel_shift);
    Serial.print(" = ");
    Serial.print(waves_exp[sel][start_bit + index - 1]);
    Serial.print(" : ");
    // Get location, true = high = 0, false = low = 60
    uint y_loc = waves_exp[sel][start_bit + index - 1] ? 0 : 63;
    int x_loc = -(pixel_shift % PIXELS_PER_BIT) + (index - 1) * PIXELS_PER_BIT +
                START_OFFSET;

    display[sel].drawLine(x_loc, y_loc, x_loc + PIXELS_PER_BIT, y_loc,
                          SSD1306_WHITE); // Draw hor line

    // If next bit is not same as current bit draw next bit
    Serial.println();
    Serial.print(waves_exp[sel].size());
    Serial.print(" : ");
    Serial.print(start_bit);
    Serial.print(" : ");
    Serial.print(index);
    Serial.println();
    if (((index + 1) < delta) && waves_exp[sel][start_bit + index - 1] !=
                                     waves_exp[sel][start_bit + index]) {
      display[sel].drawLine(x_loc + PIXELS_PER_BIT, 0, x_loc + PIXELS_PER_BIT,
                            63,
                            SSD1306_WHITE); // Draw hor line
    }
  }

  Serial.println();

  // Draw recived lines (if input OLED)
  if (sel > 3 && delta >= 1) {
    Serial.print("Drawing recoded for : ");
    Serial.println(sel);
    // Switch for State
    uint8_t draw_to = pixel_shift / 20;
    uint8_t draw_delta = START_OFFSET / 20;

    // Cap delta
    if (draw_to - draw_delta < 0)
      draw_delta = draw_to;

    uint y_loc;

    for (int8_t index = draw_to; index >= draw_to - draw_delta; index--) {
      Serial.println(index);
      Serial.print("state : ");
      Serial.println(recorded[sel - 4].getCru(index));
      switch ((State)(recorded[sel - 4].getCru(index))) {
      case TRUE:
        y_loc = 0;
        break;
      case FALSE:
        y_loc = 60;
        break;
      case FALLING:
        y_loc = 30;
        break;
      }
      Serial.println(y_loc);

      uint8_t x_loc = -pixel_shift + (index * 20) + START_OFFSET;

      display[sel].fillRect(x_loc - 1, y_loc, 4, 4, SSD1306_WHITE);
    }
  }
  Serial.println(sel);
  i2c.lock();
  display[sel].display();
  i2c.unlock();
}

void IOInterface::selOLED(uint8_t sel) {
  // Mix address to get actually address given name
  sel = mix[sel];

  Wire1.beginTransmission(0x70 /*TCA chip addr*/);
  Wire1.write(1 << sel);
  Wire1.endTransmission();
}

JsonArray j_output, j_input;

void interfaceFunc(void *parameter) { IOInterface(j_output, j_input); }

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
      oled(128 /*w*/, 32 /*h*/, &Wire, -1) {

  // Setup OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Primary SSD1306 allocation failed");
    for (;;)
      ; // Don't proceed, loop forever
  }

  bool debug = true;
  // Set rotation for messed up PCB :<
  oled.setRotation(2);

  if (!debug) { // Show welcome screen
    Serial.println("Showing welcome screen");
    oled.clearDisplay();
    oled.invertDisplay(true);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(1, 1);
    oled.println(F("Test board"));
    oled.setTextSize(1);
    oled.setCursor(70, 20);
    oled.println("Ver : 0.2");
    oled.display();
    delay(2000);
  }

  if (!debug) { // Connect to wifi and get time
    network.updateTime();
  }
  Serial.println("getting web stuff");
  Serial.println(network.getQuestions());
  Serial.println("yooo");

  if (!debug) { // Connect to wifi and pull server json
    oled.clearDisplay();
    Serial.println("Downloading questions from server");
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(1, 1);
    oled.println(F("Getting   questions"));
    oled.display();
    deserializeJson(j_questions,
                    network.getQuestions()); // Convert network String to JSON
    num_questions = j_questions["questions"].size();
  } else {
    deserializeJson(j_questions,
                    "{\"name\":\"test exam "
                    "1\",\"questions\":[{\"out\":[\"0101\",\"0011\"],\"in\":["
                    "\"0111\"]},{\"out\":[\"0101\",\"0011\"],\"in\":[\"0001\"]}"
                    ",{\"out\":[\"0101\",\"0011\"],\"in\":[\"0110\"]}]}");
    num_questions = j_questions["questions"].size();
  }

  if (!debug) { // Getting student ID from NFC
    oled.clearDisplay();
    Serial.println("Waiting for NFC student ID");
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(1, 1);
    oled.println(F("Scan ID   Card")); // weird spacing because of text roll
    oled.display();
    NFC nfc = NFC();
    student_id = nfc.getID();
    oled.clearDisplay();
    oled.setCursor(1, 1);
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
    oled.setCursor(1, 1);
    oled.print(num_questions);
    oled.println(F(" question"));
    oled.println(F("2 hours"));
    oled.display();
  }

  // { // Setup time draw function
  //   timer = timerBegin(0, 80000000, true); // Setup timer to pulse every
  //   80Mpulses (1s @ 80MHz) timerAttachInterrupt(timer,
  //   std::bind(&Interface::updateTime, this), true); // Attach our update
  //   func timerAlarmWrite(timer, 60, true); // Set timer intrupt every 60
  //   units (unit = 1s) timerAlarmEnable(timer); // enable timer
  // }

  { // Attach interrupts to rot enc
    // Attach encoders to ESP32encoder class
    encoder_q.attachHalfQuad(25, 26); // Question
    encoder_q.clearCount();           // Clear value

    // Attach button handler
    pinMode(27, INPUT);
    attachInterrupt(27, qBtnPress, FALLING); // Question
  }

  { // Start encoder UI, while loop because OOP callbacks just not worth it in
    // C
    bool init = false;             // checks if user is on launch page
    bool running = false;          // running a question
    unsigned long revert_time = 0; // used to revert to screen after 3 seconds
                                   // of non selected question
    uint8_t view_question = 0,
            sel_question = 0; // current view / current selected
    MUXTask = NULL;           // Set task null till it's used

    while (1) {
      if (revert_time && revert_time < millis()) {
        revert_time = 0;
        view_question = sel_question;     // reset to reverted
        encoder_q.setCount(sel_question); // Set encoder back to question
        updateDisplay(view_question);     // remove bar / refresh screen
        // Invert screen to show active
        oled.invertDisplay(true);
        i2c.lock();
        oled.display();
        i2c.unlock();
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
        i2c.lock();
        oled.display();
        i2c.unlock();
        // Start question
        if (MUXTask != NULL) {
          vTaskDelete(MUXTask);
        }

        // Set waves
        j_output =
            j_questions["questions"][sel_question]["out"].as<JsonArray>();
        j_input = j_questions["questions"][sel_question]["in"].as<JsonArray>();

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
        i2c.lock();
        oled.display();
        i2c.unlock();
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
  oled.setCursor(1, 1);
  oled.print(F("Question "));
  oled.println(question_num + 1);
  oled.setTextSize(1);
  oled.setCursor(1, 20);
  oled.print(F("Of "));
  oled.println(num_questions);
  i2c.lock();
  oled.display();
  i2c.unlock();
}

// void IRAM_ATTR Interface::updateTime() {}