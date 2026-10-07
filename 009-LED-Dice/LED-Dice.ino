// 009 - LED Dice
// Press the button to "roll" the dice: all seven LEDs flash briefly,
// then a random value from 1 to 6 is shown in a classic dice layout.
// The result is also printed to the Serial Monitor.
// Board: Arduino Uno
// Wiring: 7x LEDs (dice layout) each through a 220 ohm resistor to GND.
//         Push button between pin 3 and GND (internal pull-up used).
//         NOTE: pin 3 is reserved for the button, so the LED chain starts
//         at pin 2 and then skips to pins 4-9 to avoid a pin clash.

// LED pin constants arranged as: top-left, top-right, mid-left,
// centre, mid-right, bottom-left, bottom-right
const int LED_PINS[] = {2, 4, 5, 6, 7, 8, 9};
const int LED_COUNT = 7;

// Button pin (INPUT_PULLUP, so pressing it reads LOW)
const int BUTTON_PIN = 3;

// Timing constants
const unsigned long DEBOUNCE_MS = 50;   // Ignore switch bounce for 50 ms
const int FLASH_MS = 120;               // Speed of the rolling flash
const int SHOW_MS = 1500;               // How long the result stays on

// Dice patterns for values 1 to 6, one bit per LED.
// Bit order matches LED_PINS: bit0=TL, bit1=TR, bit2=ML,
// bit3=centre, bit4=MR, bit5=BL, bit6=BR
const byte DICE_PATTERNS[6] = {
  0b0001000,  // 1 -> centre
  0b0100010,  // 2 -> top-right + bottom-left
  0b0101010,  // 3 -> top-right + centre + bottom-left
  0b1100011,  // 4 -> four corners
  0b1101011,  // 5 -> four corners + centre
  0b1110111   // 6 -> four corners + middle pair
};

bool rollDone = false;              // Repeat guard: one roll per press
unsigned long lastChangeTime = 0;   // Debounce bookkeeping
int lastReading = HIGH;

void setup() {
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  // Seed the random generator from noise on the unused pin A0 so the
  // dice throws are different after every power cycle
  randomSeed(analogRead(A0));

  Serial.println("LED Dice ready. Press the button to roll.");
}

// Turn every LED on or off in one call
void setAllLeds(int state) {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], state);
  }
}

// Show one dice face (1-6) using the pattern table
void showFace(byte value) {
  byte pattern = DICE_PATTERNS[value - 1];
  for (int i = 0; i < LED_COUNT; i++) {
    // Test one bit of the pattern: HIGH if that LED should be lit
    if (pattern & (1 << i)) {
      digitalWrite(LED_PINS[i], HIGH);
    } else {
      digitalWrite(LED_PINS[i], LOW);
    }
  }
}

void loop() {
  // Debounced button read
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (reading != lastReading) {
    lastChangeTime = now;
    lastReading = reading;
  }

  bool pressed = (reading == LOW) && ((now - lastChangeTime) >= DEBOUNCE_MS);

  if (pressed && !rollDone) {
    rollDone = true;

    // Phase 1: "rolling" - flash everything a few times
    Serial.println("Rolling...");
    for (int f = 0; f < 4; f++) {
      setAllLeds(HIGH);
      delay(FLASH_MS);
      setAllLeds(LOW);
      delay(FLASH_MS);
    }

    // Phase 2: land on a random face from 1 to 6
    int result = random(1, 7);
    showFace((byte)result);

    Serial.print("Dice result: ");
    Serial.println(result);
    delay(SHOW_MS);

    setAllLeds(LOW);
  }

  // Release the guard once the button is let go, ready for the next roll
  if (reading == HIGH) {
    rollDone = false;
  }
}
