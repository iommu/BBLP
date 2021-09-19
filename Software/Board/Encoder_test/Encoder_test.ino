#include <ESP32Encoder.h>

ESP32Encoder encoder;
ESP32Encoder encoder2;

// timer and flag for example, not needed for encoders
unsigned long encoder2lastToggled;
bool encoder2Paused = false;

void sw1Press() {
  Serial.println("Switch 1 pressed!");
}

void sw2Press() {
  Serial.println("Switch 2 pressed!");
}

void setup(){
	
	Serial.begin(115200);
	// Enable the weak pull down resistors

	// use pin 19 and 18 for the first encoder
	encoder.attachHalfQuad(25, 26); // Question
	// use pin 17 and 16 for the second encoder
	encoder2.attachHalfQuad(32, 34); // Time
		
	// set starting count value after attaching
	encoder.setCount(37);

	// clear the encoder's raw count and set the tracked count to zero
	encoder2.clearCount();
	Serial.println("Encoder Start = " + String((int32_t)encoder.getCount()));

 // Setup buttons
 pinMode(27, INPUT);
 pinMode(33, INPUT);
 attachInterrupt(27, sw1Press, FALLING); // Question
 attachInterrupt(33, sw2Press, FALLING); // Time
}

void loop(){
	// Loop and read the count
	Serial.println("Encoder count = " + String((int32_t)encoder.getCount()) + " " + String((int32_t)encoder2.getCount()));
	delay(100);
//
//	// every 5 seconds toggle encoder 2
//	if (millis() - encoder2lastToggled >= 5000) {
//		if(encoder2Paused) {
//			Serial.println("Resuming Encoder 2");
//			encoder2.resumeCount();
//		} else {
//			Serial.println("Paused Encoder 2");
//			encoder2.pauseCount();
//		}
//
//		encoder2Paused = !encoder2Paused;
//		encoder2lastToggled = millis();
//	}
}
