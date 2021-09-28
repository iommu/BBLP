#include "interface.hpp"

// Non OOP stuff because callbacks, yay

std::mutex i2c; // I2C lock

uint8_t ans_corr[10] = {0}; // Answer correctness array, Max 10 questions

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

IOInterface::IOInterface(JsonArray j_output, JsonArray j_input,
                         uint8_t question_index = 0)
    : question_index(question_index), max_exp_bits(0) {
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
  }

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

  // Attach all interrupts
  encoder_t.attachHalfQuad(32, 34); // Time

  pinMode(33, INPUT);
  attachInterrupt(33, tBtnPress, FALLING); // Time

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
      t_state = false; // reset because processing
      uint8_t index;
      // Check we're still holding it for 2 seconds
      for (index = 0; index < 200; index++) {
        delay(10);
        if (digitalRead(33))
          break;
      }
      if (index >= 100) {
        Serial.println("Checking answers");
        checkQuestion();
      } else {
        Serial.println("Starting autoshift");
        auto_shift = !auto_shift;
      }
    }

    new_count = encoder_t.getCount();
    if (auto_shift || (old_count != new_count)) {
      draw8((new_count - old_count) * 20 +
            auto_shift * 5); // Encoder differential * 20 + auto * 5
      old_count = new_count;
    } else { // If not updating all, check individual
      State pins[4];
      readPins(pins);

      uint8_t write_index = pixel_shift / 20;
      uint start_bit = pixel_shift / PIXELS_PER_BIT; // First display bit
      Serial.println(start_bit);
      int delta =
          ceil((float)(128) / PIXELS_PER_BIT); // Number of bits per frame
      Serial.println(delta);
      for (uint pin_index = 0; pin_index < 4; pin_index++) {
        if ((int)pins[pin_index] != recorded[pin_index].getCru(write_index)) {
          recorded[0].setCru(write_index, pins[pin_index]);
          draw(pin_index + 4, start_bit, delta);
        }
      }
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

    uint y_loc = 0;

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

void IOInterface::checkQuestion() {
  ans_corr[question_index] = 0; // set max
  for (uint8_t index = 0; index < max_exp_bits; index++) {
    State pins[] = {(State)waves_exp[0][index], (State)waves_exp[1][index],
                    (State)waves_exp[2][index], (State)waves_exp[3][index]};
    writePins(pins);
    readPins(pins);
    Serial.print("Pass ");
    Serial.print(index);
    if ((!waves_exp[4].size() || (State)waves_exp[4][index] == pins[0]) &&
        (!waves_exp[5].size() || (State)waves_exp[5][index] == pins[1]) &&
        (!waves_exp[6].size() || (State)waves_exp[6][index] == pins[2]) &&
        (!waves_exp[7].size() || (State)waves_exp[7][index] == pins[3])) {
      // Correct
      Serial.println(" Correct!");
      ans_corr[question_index]++;
    } else {
      // Incorr
      Serial.println(" Fail!");
    }
  }
  // average
  Serial.println(ans_corr[question_index]);
  ans_corr[question_index] =
      ((float)ans_corr[question_index] / max_exp_bits) * 100;
  Serial.println(ans_corr[question_index]);
}

JsonArray j_output, j_input;
uint8_t question_index;

void interfaceFunc(void *parameter) {
  IOInterface(j_output, j_input, question_index);
}

// Interface

bool q_state = false;

void qBtnPress() { q_state = true; }

Interface::Interface()
    : ind_led(), network(), j_exam(2048), oled(128 /*w*/, 32 /*h*/, &Wire, -1) {

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

  // Connect to wifi and get time
  network.updateTime();

  if (!debug) { // Connect to wifi and pull server json
    oled.clearDisplay();
    Serial.println("Downloading questions from server");
    oled.invertDisplay(false);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(2);
    oled.setCursor(1, 1);
    oled.println(F("Getting   questions"));
    oled.display();
    deserializeJson(j_exam,
                    network.getQuestions()); // Convert network String to JSON
    exam_questions = j_exam["questions"].size();
  } else {
    deserializeJson(j_exam,
                    "{\"name\":\"test exam "
                    "1\",\"time\": \"20\", "
                    "\"questions\":[{\"out\":[\"0101\",\"0011\"],\"in\":["
                    "\"0111\"]},{\"out\":[\"0101\",\"0011\"],\"in\":[\"0001\"]}"
                    ",{\"out\":[\"0101\",\"0011\"],\"in\":[\"0110\"]}]}");
    Serial.print("Getting no. questions : ");
    exam_questions = j_exam["questions"].size();
    Serial.println(exam_questions);
    Serial.print("Getting time : ");
    exam_time = j_exam["time"].as<uint8_t>();
    Serial.println(exam_time);
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
    oled.print(exam_questions);
    oled.println(F(" question"));
    oled.print(exam_time);
    oled.print(" minutes");
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

  { // Start encoder UI, while loop because OOP callbacks just not worth it with
    // C callbacks :<
    bool init = false;             // checks if user is on launch page
    bool running = false;          // running a question
    unsigned long revert_time = 0; // used to revert to screen after 3 seconds
                                   // of non selected question
    uint8_t view_page = 0,
            sel_question = 0; // current view / current selected
    MUXTask = NULL;           // Set task null till it's used

    uint8_t cur_perc = 0; // current question percentage
    while (1) {
      if (revert_time && revert_time < millis()) {
        revert_time = 0;
        view_page = sel_question;         // reset to reverted
        encoder_q.setCount(sel_question); // Set encoder back to question
        updateDisplay(view_page);         // remove bar / refresh screen
        // Invert screen to show active
        oled.invertDisplay(true);
        i2c.lock();
        oled.display();
        i2c.unlock();
      } else if ((q_state || cur_perc != ans_corr[sel_question]) && init) {
        // Check if we're on "special interface screen"
        if (encoder_q.getCount() % (exam_questions + 1) == exam_questions) {
          oled.clearDisplay();
          oled.invertDisplay(true);
          oled.setTextColor(SSD1306_WHITE);
          oled.setTextSize(2);
          oled.setCursor(1, 1);
          oled.println(F("Saving!"));
          oled.setTextSize(1);
          oled.println("Please wait :)");
          i2c.lock();
          oled.display();
          i2c.unlock();
          delay(1000);
          // network.uploadAnswers();
          revert_time = 1; // force revert
        } else {
          cur_perc = ans_corr[sel_question]; // update percentage
          q_state = false;                   // reset button
          running = true;                    // we are now running a question
          revert_time = 0;          // reset revert time if selected question
          sel_question = view_page; // we clicked = we want
          encoder_q.setCount(sel_question); // Set encoder back to question
                                            // (incase changed during push)
          updateDisplay(view_page);         // remove bar / refresh screen

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
          j_output = j_exam["questions"][sel_question]["out"].as<JsonArray>();
          j_input = j_exam["questions"][sel_question]["in"].as<JsonArray>();
          question_index = sel_question;

          xTaskCreatePinnedToCore(
              interfaceFunc,   /* Function to implement the task */
              "mux interface", /* Name of the task */
              10000,           /* Stack size in words */
              NULL,            /* Task input parameter */
              0,               /* Priority of the task */
              &MUXTask,        /* Task handle. */
              1);              /* Core where the task should run */
        }
      } else if (encoder_q.getCount() % (exam_questions + 1) != view_page) {
        if (running) {
          revert_time = millis() + 3000; // revert in 3 seconds past now
        }
        if (!init) {
          init = true;           // We've changed the variable so let us use it
          encoder_q.setCount(0); // reset to 0
        }
        // Wrap if neg
        if (encoder_q.getCount() < 0) {
          encoder_q.setCount(exam_questions - 1);
        }

        // Change page
        view_page = encoder_q.getCount() % (exam_questions + 1);

        // Check if we're on "special interface screen"
        if (view_page == exam_questions) {
          oled.clearDisplay();
          oled.invertDisplay(false);
          oled.setTextColor(SSD1306_WHITE);
          oled.setTextSize(2);
          oled.setCursor(1, 1);
          oled.println(F("Save?"));
          oled.setTextSize(1);
          oled.print(exam_time);
          oled.println(F("m remaining"));
          i2c.lock();
          oled.display();
          i2c.unlock();
        } else {
          updateDisplay(view_page);
        }
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
  oled.print(exam_questions);
  oled.print("      ");
  oled.print(ans_corr[question_num]);
  oled.print("%");
  oled.println(" done!");
  ind_led.setRGB(250 - 25 * ans_corr[question_num], 25 * ans_corr[question_num],
                 0);
  ind_led.setRGB(0, 0, 0);
  i2c.lock();
  oled.display();
  i2c.unlock();
}

// void IRAM_ATTR Interface::updateTime() {}