// 002 - Multiple LED Sequence
// Six LEDs connected to pins 2-7 light up one after another like a "chaser",
// then the pattern runs backwards.
// Board: Arduino Uno
// Wiring: LED anodes -> pins 2,3,4,5,6,7 through 220 ohm resistors, cathodes -> GND

// Number of LEDs and the first pin they are connected to
const int LED_COUNT = 6;
const int FIRST_LED_PIN = 2;   // LEDs use pins 2,3,4,5,6,7
const int STEP_MS = 150;       // Delay between each LED step (milliseconds)

void setup() {
  // Configure every LED pin as an OUTPUT in one loop
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(FIRST_LED_PIN + i, OUTPUT);
  }

  Serial.begin(9600);
  Serial.println("Multiple LED Sequence starting...");
}

void loop() {
  // Forward chase: turn LEDs on one by one from pin 2 to pin 7
  Serial.println("Forward chase");
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(FIRST_LED_PIN + i, HIGH);  // Light the current LED
    delay(STEP_MS);                          // Hold the pattern briefly
    digitalWrite(FIRST_LED_PIN + i, LOW);    // Turn it off again
  }

  // Reverse chase: run the same pattern from pin 7 back to pin 2
  Serial.println("Reverse chase");
  for (int i = LED_COUNT - 1; i >= 0; i--) {
    digitalWrite(FIRST_LED_PIN + i, HIGH);
    delay(STEP_MS);
    digitalWrite(FIRST_LED_PIN + i, LOW);
  }
}
