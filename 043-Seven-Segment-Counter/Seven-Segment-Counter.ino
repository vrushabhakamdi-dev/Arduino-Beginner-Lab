// 043 - Seven Segment Counter
// One push button counts 0 to 9 on a common-cathode 7-segment display that is
// driven through a 74HC595 shift register (only 3 Arduino pins are needed).
// Board: Arduino Uno
//
// Wiring:
//   Button        -> pin 2 to GND (internal pull-up is used)
//   74HC595  DS   (pin 14) -> Arduino pin 4
//   74HC595  SHCP (pin 11) -> Arduino pin 5  (serial clock)
//   74HC595  STCP (pin 12) -> Arduino pin 6  (latch / storage clock)
//   74HC595  OE   (pin 13) -> GND            (output enable, always on)
//   74HC595  MR   (pin 10) -> 5V             (master reset, inactive)
//   74HC595  Q0..Q7 -> segments a,b,c,d,e,f,g,dp through 220 ohm resistors
//   Common cathode leg of the display -> GND

// ---------------------------------------------------------------------------
// Pin constants (named constants instead of magic numbers)
// ---------------------------------------------------------------------------
const uint8_t DATA_PIN = 4;    // 74HC595 DS  - serial data in
const uint8_t CLOCK_PIN = 5;   // 74HC595 SHCP - shift clock
const uint8_t LATCH_PIN = 6;   // 74HC595 STCP - latch clock
const uint8_t BUTTON_PIN = 2;  // push button to GND

const unsigned long DEBOUNCE_MS = 30;  // ignore pulses shorter than 30 ms

// ---------------------------------------------------------------------------
// Digit decode table for a COMMON CATHODE display.
// Bit 0 = segment a, bit 1 = b, ... bit 6 = g, bit 7 = decimal point.
//           a
//          ---
//       f |   | b
//          -g-
//       e |   | c
//          ---
//           d
// ---------------------------------------------------------------------------
const byte SEGMENT_TABLE[10] = {
  0b00111111,  // 0 -> a b c d e f
  0b00000110,  // 1 -> b c
  0b01011011,  // 2 -> a b d e g
  0b01001111,  // 3 -> a b c d g
  0b01100110,  // 4 -> b c f g
  0b01101101,  // 5 -> a c d f g
  0b01111101,  // 6 -> a c d e f g
  0b00000111,  // 7 -> a b c
  0b01111111,  // 8 -> all segments
  0b01101111   // 9 -> a b c d f g
};

const byte BLANK = 0b00000000;  // all segments off

// ---------------------------------------------------------------------------
// Button state
// ---------------------------------------------------------------------------
uint8_t count = 0;                // the number currently shown (0..9)
int buttonState = HIGH;           // debounced state, LOW = pressed
int lastReading = HIGH;           // raw reading from the previous loop
unsigned long lastDebounceMs = 0; // time of the last change

// ---------------------------------------------------------------------------
// Push one byte into the shift register and latch it onto the outputs.
// shiftOut() sends the most significant bit first, so bit 7 (decimal point)
// leaves the chip first and lands on the Q7 output.
// ---------------------------------------------------------------------------
void showSegments(byte segments) {
  digitalWrite(LATCH_PIN, LOW);   // open the latch so new data can enter
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, segments);
  digitalWrite(LATCH_PIN, HIGH);  // latch: copy the shift register to the outputs
}

// ---------------------------------------------------------------------------
// Show a single digit 0..9 on the display
// ---------------------------------------------------------------------------
void showDigit(uint8_t digit) {
  showSegments(SEGMENT_TABLE[digit % 10]);
}

// ---------------------------------------------------------------------------
// Debounced button check.
// Returns true exactly once per physical press (rising edge after release).
// ---------------------------------------------------------------------------
bool buttonPressed() {
  bool newlyPressed = false;
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {         // any change restarts the timer
    lastDebounceMs = millis();
  }
  lastReading = reading;

  if ((millis() - lastDebounceMs) >= DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {         // button pulls the pin to GND
        newlyPressed = true;
      }
    }
  }
  return newlyPressed;
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("043 - Seven Segment Counter starting..."));

  // The shift register needs three output pins
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  // The button uses the internal pull-up resistor, no external resistor needed
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  buttonState = digitalRead(BUTTON_PIN);
  lastReading = buttonState;

  showDigit(count);
  Serial.print(F("Count = "));
  Serial.println(count);
}

void loop() {
  // --- Button handling ---------------------------------------------------
  if (buttonPressed()) {
    // Wrap around: after 9 the next press shows 0 again
    count = (count + 1) % 10;
    showDigit(count);

    Serial.print(F("Button pressed -> count = "));
    Serial.println(count);
  }

  // Small pause keeps the loop gentle; the debounce already blocks re-triggers
  delay(5);
}
