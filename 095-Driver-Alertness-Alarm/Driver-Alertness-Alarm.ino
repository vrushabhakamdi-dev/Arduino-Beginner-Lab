/*
 * 095 - Driver Alertness Alarm
 * Board: Arduino Uno
 *
 * DROWSINESS SIMULATION. The "driver" must tap the button on pin 2 once in
 * a while to prove they are still attentive. If no tap arrives within the
 * timeout window, the alertness alarm sounds: buzzer pin 8 beeps in a
 * repeating pattern and the LED on pin 13 flashes, and both keep running
 * until the driver taps the button again to acknowledge the alert.
 *
 * A potentiometer on A0 sets the timeout window to anywhere between
 * MIN_TIMEOUT_MIN and MAX_TIMEOUT_MIN minutes (3 to 10 minutes by default).
 *
 * IMPORTANT: this sketch demonstrates TIMING LOGIC only - millis()-based
 * deadlines, latching alarms and user acknowledgement. It is NOT a real
 * vehicle safety device and must never be installed in a vehicle or relied
 * upon to prevent an accident.
 *
 * Wiring:
 *   Driver button -> digital pin 2 -> GND (internal pull-up used)
 *   Potentiometer: left pin -> GND, right pin -> 5V, wiper -> analog A0
 *   LED + 220 ohm -> digital pin 13 (built-in LED is fine)
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *
 * BENCH TEST TIP: waiting 3-10 minutes is slow on a desk, so temporarily
 * change MIN_TIMEOUT_MIN / MAX_TIMEOUT_MIN to 1 and 2 while testing.
 */

// ---------------- Named pin constants ----------------
const int DRIVER_BUTTON_PIN = 2;   // driver taps this to prove alertness
const int TIMEOUT_POT_PIN = A0;    // sets the allowed idle time
const int LED_PIN = 13;            // flashes while the alarm runs
const int BUZZER_PIN = 8;          // alertness buzzer

// ---------------- Alertness window (minutes) ----------------
const int MIN_TIMEOUT_MIN = 3;    // pot fully counter-clockwise
const int MAX_TIMEOUT_MIN = 10;   // pot fully clockwise

// ---------------- Alarm pattern timing ----------------
const unsigned long BEEP_MS = 300;    // tone on
const unsigned long GAP_MS = 200;     // silence between beeps
const unsigned long DEBOUNCE_MS = 40; // button stability time
const unsigned long STATUS_MS = 15000; // progress line every 15 s

// ---------------- State ----------------
unsigned long lastInputMs = 0;     // when the driver last tapped the button
unsigned long alertStartMs = 0;    // when the current alarm began
int timeoutMinutes = 4;            // current pot setting
bool alertActive = false;          // alarm latched until acknowledged
int alertCount = 0;                // total alerts raised
unsigned long lastStatusMs = 0;

int lastRawButton = HIGH;
int lastStableButton = HIGH;
unsigned long lastButtonChangeMs = 0;

// Debounced button: true once per clean press.
bool driverTapped() {
  int raw = digitalRead(DRIVER_BUTTON_PIN);
  unsigned long now = millis();
  if (raw != lastRawButton) {
    lastRawButton = raw;
    lastButtonChangeMs = now;
  }
  if (now - lastButtonChangeMs >= DEBOUNCE_MS && raw != lastStableButton) {
    lastStableButton = raw;
    if (raw == LOW) return true;
  }
  return false;
}

// Read the potentiometer and convert it to the allowed idle time in minutes.
int readTimeoutFromPot() {
  int raw = analogRead(TIMEOUT_POT_PIN);                       // 0..1023
  return map(raw, 0, 1023, MIN_TIMEOUT_MIN, MAX_TIMEOUT_MIN);  // 3..10 min
}

// Seconds -> "4 min 05 s" style text, printed for the log.
void printElapsed(unsigned long ms) {
  unsigned long totalSeconds = ms / 1000;
  Serial.print(totalSeconds / 60);
  Serial.print(" min ");
  if (totalSeconds % 60 < 10) Serial.print('0');
  Serial.print(totalSeconds % 60);
  Serial.print(" s");
}

void setup() {
  Serial.begin(9600);
  pinMode(DRIVER_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  timeoutMinutes = readTimeoutFromPot();
  lastInputMs = millis();   // start the clock at power-up

  Serial.println("Driver Alertness Alarm ready (timing demonstration only).");
  Serial.print("Tap the button on pin 2 at least every ");
  Serial.print(timeoutMinutes);
  Serial.println(" minutes.");
  Serial.println("Move the potentiometer on A0 to change the window (3-10 min).");
}

void loop() {
  unsigned long now = millis();

  // ---- 1. Potentiometer can change the window at any time ----
  int newTimeout = readTimeoutFromPot();
  if (newTimeout != timeoutMinutes) {
    timeoutMinutes = newTimeout;
    Serial.print("Timeout window changed to ");
    Serial.print(timeoutMinutes);
    Serial.println(" minutes (potentiometer).");
  }

  // ---- 2. A tap either acknowledges an alarm or refreshes the timer ----
  if (driverTapped()) {
    if (alertActive) {
      alertActive = false;
      noTone(BUZZER_PIN);
      digitalWrite(LED_PIN, LOW);
      lastInputMs = now;            // restart the window after acknowledging
      Serial.print("Alert ACKNOWLEDGED after ");
      printElapsed(now - alertStartMs);
      Serial.println(". Timer restarted.");
    } else {
      lastInputMs = now;
      Serial.print("Driver input OK at t=");
      Serial.print(now / 1000);
      Serial.println(" s - timer restarted.");
    }
  }

  // ---- 3. Did the driver go quiet for longer than the window? ----
  unsigned long silentMs = now - lastInputMs;
  if (!alertActive && silentMs >= (unsigned long)timeoutMinutes * 60000UL) {
    alertActive = true;
    alertStartMs = now;
    alertCount++;

    Serial.print("*** ALERT #");
    Serial.print(alertCount);
    Serial.print(" - no driver input for ");
    printElapsed(silentMs);
    Serial.print(" (limit ");
    Serial.print(timeoutMinutes);
    Serial.println(" min). Tap the button to acknowledge. ***");
  }

  // ---- 4. Beeping alarm pattern, generated without blocking ----
  if (alertActive) {
    // One beep cycle lasts BEEP_MS + GAP_MS; the position inside the cycle
    // decides whether the tone and LED are on. No delay() is used, so the
    // button and the potentiometer keep being scanned every pass.
    unsigned long positionInCycle = now % (BEEP_MS + GAP_MS);
    if (positionInCycle < BEEP_MS) {
      tone(BUZZER_PIN, 1000);
      digitalWrite(LED_PIN, HIGH);
    } else {
      noTone(BUZZER_PIN);
      digitalWrite(LED_PIN, LOW);
    }
  }

  // ---- 5. Progress line so the user can see the countdown ----
  if (now - lastStatusMs >= STATUS_MS) {
    lastStatusMs = now;
    Serial.print("Time since last input: ");
    printElapsed(silentMs);
    Serial.print(" / limit ");
    Serial.print(timeoutMinutes);
    Serial.print(" min | state: ");
    Serial.println(alertActive ? "ALARM (tap to acknowledge)" : "watching");
  }

  delay(10);
}
