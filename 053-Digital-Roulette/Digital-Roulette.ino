// 053 - Digital Roulette
// A row of eight LEDs chases around like the pockets of a roulette wheel,
// speeding up and slowing down at random intervals. Press the button to stop
// the chase - whichever LED is lit when the wheel stops is the winning pocket.
// Board: Arduino Uno
//
// Wiring:
//   8 LEDs -> pins 2, 3, 4, 5, 6, 7, 8 and 9 (anode to the pin through a
//             220 ohm resistor, cathode to GND)
//   Stop / start button -> pin 10 (other leg to GND, internal pull-up used)
//   Passive buzzer (+) -> pin 11, buzzer (-) -> GND
//   NOTE: pin 8 is already used by one of the LEDs, so the buzzer sits on pin 11.

// ------------------------------ Pin constants ------------------------------
const byte LED_COUNT = 8;
const byte LED_PIN[LED_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9};  // the "pockets"
const byte STOP_BUTTON_PIN = 10;                            // stops / starts the wheel
const byte BUZZER_PIN = 11;                                 // blip when a winner is found

// ------------------------------ Timing constants ---------------------------
const unsigned long DEBOUNCE_MS = 30;      // button debounce window
const unsigned long MIN_INTERVAL_MS = 40;  // fastest chase step (wheel spinning fast)
const unsigned long MAX_INTERVAL_MS = 400; // slowest chase step (wheel almost stopped)
const unsigned long SLOW_DOWN_MS = 8;      // extra delay added every step (friction)
const int WIN_BLIP_HZ = 2000;              // pitch of the winner blip

// ------------------------------ Runtime state ------------------------------
bool spinning = false;            // true while the wheel is running
byte currentIndex = 0;            // which LED is lit right now
unsigned long nextStepAt = 0;     // millis() time of the next chase step
unsigned long stepsTaken = 0;     // used to slow the wheel down over time

// Button debounce variables
bool buttonRaw;
bool buttonStable;
unsigned long buttonLastChange;

// Returns true once, on the moment the button is pressed
bool buttonPressed(unsigned long now) {
  bool reading = digitalRead(STOP_BUTTON_PIN);  // LOW = pressed (internal pull-up)

  if (reading != buttonRaw) {
    buttonRaw = reading;
    buttonLastChange = now;
  }

  if ((now - buttonLastChange) >= DEBOUNCE_MS && reading != buttonStable) {
    buttonStable = reading;
    if (buttonStable == LOW) return true;
  }
  return false;
}

// Lights only the LED given by index and switches all the others off
void showOnly(byte index) {
  for (byte i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PIN[i], (i == index) ? HIGH : LOW);
  }
}

void startSpin(unsigned long now) {
  spinning = true;
  stepsTaken = 0;
  currentIndex = 0;
  nextStepAt = now;  // take the first step immediately
  Serial.println("Wheel spinning... press the button to stop it!");
}

void stopSpin(unsigned long now) {
  spinning = false;

  // The LED that is lit at this instant decides the winner
  tone(BUZZER_PIN, WIN_BLIP_HZ, 120);  // short blip

  Serial.print("WINNER: LED on pin ");
  Serial.print(LED_PIN[currentIndex]);
  Serial.print("  (pocket ");
  Serial.print(currentIndex + 1);
  Serial.println(" of 8)");
  Serial.println("Press the button again to spin once more.");
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
  }

  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);  // button only needs to short the pin to GND
  buttonRaw = digitalRead(STOP_BUTTON_PIN);
  buttonStable = buttonRaw;
  buttonLastChange = millis();

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.println("Digital Roulette ready.");
  Serial.println("Press the button on pin 10 to spin, then press again to stop.");
}

void loop() {
  unsigned long now = millis();

  // --- Button handling works both while spinning and while idle ---
  if (buttonPressed(now)) {
    if (spinning) {
      stopSpin(now);
    } else {
      startSpin(now);
    }
  }

  if (!spinning) return;  // wheel is stopped, just keep watching the button

  // --- Advance the chase when it is time ---
  if (now >= nextStepAt) {
    currentIndex = (currentIndex + 1) % LED_COUNT;  // wrap around from 9 back to 2
    showOnly(currentIndex);

    // Randomizing interval: a random base plus a delay that grows every step,
    // which makes the wheel start fast and gradually slow down like a real one
    unsigned long interval = MIN_INTERVAL_MS + stepsTaken * SLOW_DOWN_MS;
    if (interval > MAX_INTERVAL_MS) interval = MAX_INTERVAL_MS;
    interval += random(0, 60);  // a little randomness on top of every step

    nextStepAt = now + interval;
    stepsTaken++;
  }
}
