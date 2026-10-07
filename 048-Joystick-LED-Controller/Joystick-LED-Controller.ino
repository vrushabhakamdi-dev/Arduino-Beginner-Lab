// 048 - Joystick LED Controller
// Move the joystick left/right and one of eight LEDs lights up. Press the
// joystick button to switch into a running-chase mode.
// Board: Arduino Uno
//
// Wiring:
//   Joystick module  -> 5V, GND
//        VRx (X)  -> A0
//        VRy (Y)  -> A1
//        SW (push)-> pin 2 to GND (internal pull-up is used)
//   LED 1..8 -> pins 3,4,5,6,7,8,9,10, each through a 220 ohm resistor to GND

// ---------------------------------------------------------------------------
// Pin constants
// ---------------------------------------------------------------------------
const uint8_t JOYSTICK_X_PIN = A0;     // horizontal position, 0..1023
const uint8_t JOYSTICK_Y_PIN = A1;     // vertical position, 0..1023
const uint8_t JOYSTICK_SW_PIN = 2;     // joystick push button

const uint8_t LED_COUNT = 8;           // number of LEDs in the row
const uint8_t LED_PINS[LED_COUNT] = {3, 4, 5, 6, 7, 8, 9, 10};

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
const unsigned long DEBOUNCE_MS = 30;      // button debounce time
const unsigned long SERIAL_MS = 300;       // print X/Y only 3 times a second
const unsigned long CHASE_MS = 120;        // speed of the chase animation
const int DEAD_ZONE = 40;                  // ignore tiny joystick wobble (±40)

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
bool chaseMode = false;            // false = joystick follows, true = chasing
uint8_t chaseIndex = 0;            // which LED is lit during the chase
unsigned long lastChaseMs = 0;
unsigned long lastSerialMs = 0;

int buttonState = HIGH;            // debounced state, LOW = pressed
int lastReading = HIGH;
unsigned long lastDebounceMs = 0;

// ---------------------------------------------------------------------------
// Turn off all LEDs except the one at 'activeIndex'.
// Use activeIndex = LED_COUNT to switch every LED off.
// ---------------------------------------------------------------------------
void lightOnly(uint8_t activeIndex) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], (i == activeIndex) ? HIGH : LOW);
  }
}

// ---------------------------------------------------------------------------
// Debounced button check, returns true once per press
// ---------------------------------------------------------------------------
bool buttonPressed() {
  bool newlyPressed = false;
  int reading = digitalRead(JOYSTICK_SW_PIN);

  if (reading != lastReading) lastDebounceMs = millis();
  lastReading = reading;

  if ((millis() - lastDebounceMs) >= DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) newlyPressed = true;
    }
  }
  return newlyPressed;
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("048 - Joystick LED Controller starting..."));

  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  digitalWrite(LED_PINS[0], HIGH);   // start with the first LED lit

  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
  buttonState = digitalRead(JOYSTICK_SW_PIN);
  lastReading = buttonState;

  Serial.println(F("Move the joystick to pick an LED, press it for chase mode."));
}

void loop() {
  // --- Read the analog joystick axes -------------------------------------
  int xValue = analogRead(JOYSTICK_X_PIN);   // 0 = full left, 1023 = full right
  int yValue = analogRead(JOYSTICK_Y_PIN);   // 0 = full up,   1023 = full down

  // --- Button toggles between the two modes ------------------------------
  if (buttonPressed()) {
    chaseMode = !chaseMode;
    lastChaseMs = millis();
    chaseIndex = 0;
    Serial.println(chaseMode ? F("MODE: LED chase") : F("MODE: joystick follow"));
  }

  if (chaseMode) {
    // --- Chase mode: light the next LED every CHASE_MS milliseconds -----
    if (millis() - lastChaseMs >= CHASE_MS) {
      lastChaseMs += CHASE_MS;
      chaseIndex = (chaseIndex + 1) % LED_COUNT;
      lightOnly(chaseIndex);
    }
  } else {
    // --- Follow mode: X position chooses the LED ------------------------
    // Dead zone in the middle so the LEDs do not flicker when hands are off
    int litLed = -1;
    if (abs(xValue - 512) > DEAD_ZONE) {
      // map the analog range 0..1023 onto LED 0..7
      litLed = map(xValue, 0, 1023, 0, LED_COUNT - 1);
      litLed = constrain(litLed, 0, LED_COUNT - 1);
    }
    if (litLed >= 0) {
      lightOnly((uint8_t)litLed);
    }
  }

  // --- Periodic Serial report -------------------------------------------
  if (millis() - lastSerialMs >= SERIAL_MS) {
    lastSerialMs = millis();
    Serial.print(F("X = "));
    Serial.print(xValue);
    Serial.print(F("   Y = "));
    Serial.print(yValue);
    Serial.print(F("   mode = "));
    Serial.println(chaseMode ? F("chase") : F("follow"));
  }
}
