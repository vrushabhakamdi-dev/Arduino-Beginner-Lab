/*
 * 092 - Emergency Siren System
 * Board: Arduino Uno
 *
 * Three classic emergency siren patterns - WAIL, YELP and HI-LO - are
 * generated on a passive buzzer. One push button selects each pattern, and
 * pressing the button of the running pattern again switches the siren off.
 * The LED on pin 13 flashes in sync with the tone so the alarm is visible
 * as well as audible, and the Serial Monitor reports the current mode.
 *
 * Wiring:
 *   Push button WAIL -> digital pin 2 -> GND   (internal pull-up used)
 *   Push button YELP -> digital pin 3 -> GND
 *   Push button HI-LO -> digital pin 4 -> GND
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *   LED + 220 ohm resistor -> digital pin 13 (built-in LED also used)
 *
 * WHY millis() INSTEAD OF delay():
 *   With delay() the sketch is stuck inside the delay call, so button
 *   presses are only noticed between delays and the tone can never be
 *   re-tuned smoothly. Every pass through loop() this sketch recalculates
 *   the tone frequency and the LED from the elapsed time (millis()), so
 *   buttons stay responsive and the sweep is perfectly smooth. This is the
 *   standard non-blocking timing technique.
 */

// ---------------- Named pin constants ----------------
const int WAIL_BUTTON_PIN = 2;   // select the WAIL pattern
const int YELP_BUTTON_PIN = 3;   // select the YELP pattern
const int HILO_BUTTON_PIN = 4;   // select the HI-LO pattern
const int BUZZER_PIN = 8;        // passive buzzer (tone() drives it)
const int LED_PIN = 13;          // flashes in sync with the siren

const int NUM_MODES = 3;
const int BUTTON_PINS[NUM_MODES] = {WAIL_BUTTON_PIN, YELP_BUTTON_PIN, HILO_BUTTON_PIN};

// ---------------- Pattern timing and pitch ----------------
const unsigned long WAIL_PERIOD_MS = 2000;  // one slow up-and-down sweep = 2 s
const unsigned long YELP_PERIOD_MS = 400;   // fast up-and-down sweep
const unsigned long HILO_HALF_MS = 500;     // each tone of the HI-LO pair

const int WAIL_LOW_HZ = 600;
const int WAIL_HIGH_HZ = 1400;
const int YELP_LOW_HZ = 900;
const int YELP_HIGH_HZ = 1500;
const int HILO_LOW_HZ = 700;
const int HILO_HIGH_HZ = 1300;

const unsigned long DEBOUNCE_MS = 40;   // buttons must be stable this long

// ---------------- State ----------------
enum SirenMode { MODE_OFF, MODE_WAIL, MODE_YELP, MODE_HILO };
SirenMode currentMode = MODE_OFF;
unsigned long phaseStartMs = 0;   // when the current pattern started

// Debounce bookkeeping, one slot per button
int lastRawState[NUM_MODES];
int lastStableState[NUM_MODES];
unsigned long lastChangeMs[NUM_MODES];
bool pressEvent[NUM_MODES] = {false, false, false};

const char *modeName(SirenMode m) {
  switch (m) {
    case MODE_WAIL: return "WAIL";
    case MODE_YELP: return "YELP";
    case MODE_HILO: return "HI-LO";
    default:        return "OFF";
  }
}

// Poll all three buttons and raise pressEvent[i] once per clean press.
// The buttons wire to GND and use the internal pull-ups, so pressed = LOW.
void readButtons() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_MODES; i++) {
    int raw = digitalRead(BUTTON_PINS[i]);
    if (raw != lastRawState[i]) {          // raw level changed -> restart timer
      lastRawState[i] = raw;
      lastChangeMs[i] = now;
    }
    if (now - lastChangeMs[i] >= DEBOUNCE_MS && raw != lastStableState[i]) {
      lastStableState[i] = raw;
      if (raw == LOW) pressEvent[i] = true;   // stable press (not release)
    }
  }
}

// Recalculate tone and LED for the running pattern.
// Everything is derived from elapsed time, so nothing ever blocks.
void updateSiren() {
  if (currentMode == MODE_OFF) return;

  unsigned long elapsed = millis() - phaseStartMs;
  int freq = 0;
  bool ledOn = false;

  if (currentMode == MODE_WAIL) {
    // Slow triangle sweep: up for half a period, then down again.
    unsigned long pos = elapsed % WAIL_PERIOD_MS;
    unsigned long half = WAIL_PERIOD_MS / 2;
    if (pos < half) {
      freq = map(pos, 0, half, WAIL_LOW_HZ, WAIL_HIGH_HZ);
    } else {
      freq = map(pos, half, WAIL_PERIOD_MS, WAIL_HIGH_HZ, WAIL_LOW_HZ);
    }
    ledOn = freq >= (WAIL_LOW_HZ + WAIL_HIGH_HZ) / 2;   // lit on the upper half

  } else if (currentMode == MODE_YELP) {
    // Same triangle shape but four times faster - the classic yelp.
    unsigned long pos = elapsed % YELP_PERIOD_MS;
    unsigned long half = YELP_PERIOD_MS / 2;
    if (pos < half) {
      freq = map(pos, 0, half, YELP_LOW_HZ, YELP_HIGH_HZ);
    } else {
      freq = map(pos, half, YELP_PERIOD_MS, YELP_HIGH_HZ, YELP_LOW_HZ);
    }
    ledOn = freq >= (YELP_LOW_HZ + YELP_HIGH_HZ) / 2;

  } else if (currentMode == MODE_HILO) {
    // Two fixed notes alternating every 500 ms.
    bool highTone = ((elapsed / HILO_HALF_MS) % 2) == 0;
    freq = highTone ? HILO_HIGH_HZ : HILO_LOW_HZ;
    ledOn = highTone;   // LED follows the high note exactly
  }

  tone(BUZZER_PIN, freq);
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
}

void startMode(SirenMode m) {
  currentMode = m;
  phaseStartMs = millis();
  Serial.print("Mode: ");
  Serial.println(modeName(m));
}

void stopSiren() {
  currentMode = MODE_OFF;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Mode: OFF - siren silent.");
}

void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  // Internal pull-ups mean the buttons only need to short the pin to GND.
  for (int i = 0; i < NUM_MODES; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastRawState[i] = HIGH;
    lastStableState[i] = HIGH;
    lastChangeMs[i] = 0;
  }

  Serial.println("Emergency Siren System ready.");
  Serial.println("Press button pin 2 = WAIL, pin 3 = YELP, pin 4 = HI-LO.");
  Serial.println("Press the running pattern's button again to stop.");
}

void loop() {
  readButtons();

  // Map each button to its pattern: 0 = WAIL, 1 = YELP, 2 = HI-LO.
  SirenMode requested[NUM_MODES] = {MODE_WAIL, MODE_YELP, MODE_HILO};

  for (int i = 0; i < NUM_MODES; i++) {
    if (pressEvent[i]) {
      pressEvent[i] = false;
      if (currentMode == requested[i]) {
        stopSiren();                 // same button again = off
      } else {
        startMode(requested[i]);     // switch to the new pattern
      }
    }
  }

  updateSiren();   // retune the tone and LED from the elapsed time

  delay(5);   // short settle only - tone generation itself never blocks
}
