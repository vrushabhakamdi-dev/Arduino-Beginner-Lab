/*
 * 093 - Smoke Detection Alarm
 * Board: Arduino Uno
 *
 * An MQ-2 smoke/gas sensor watches the air. When its digital output trips,
 * the alarm LATCHES ON: the buzzer on pin 8 and the red LED on pin 6 stay on
 * continuously - they do NOT switch off again by themselves - until somebody
 * presses the reset button on pin 3. That latching behaviour is what a real
 * smoke alarm does, so nobody can miss a short smoke event.
 *
 * The green LED on pin 4 is the power/armed indicator and is on whenever the
 * sketch is running normally.
 *
 * Wiring:
 *   MQ-2 module VCC -> 5V, GND -> GND
 *   MQ-2 module DO  -> digital pin 2 (comparator threshold output)
 *   MQ-2 module AO  -> analog pin A0 (raw sensor value, also monitored)
 *   Reset push button -> digital pin 3 -> GND (internal pull-up used)
 *   Green LED + 220 ohm -> digital pin 4 (power / armed)
 *   Red LED   + 220 ohm -> digital pin 6 (alarm)
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *
 * SENSOR WARM-UP (important!):
 *   The MQ-2 heating element must burn in for 24 to 48 hours the first time
 *   it is powered. Until then the readings drift and the threshold may trip
 *   on its own. Leave the module powered for a day or two before trusting
 *   the alarm, and turn the tiny potentiometer on the module so the DO line
 *   stays HIGH (no smoke) in clean air.
 *
 * NOTE: this project is a latching smoke alarm. Project 030 (Gas Leak
 * Alarm) is an analog gas monitor that prints concentration levels - this
 * one is a digital, latching alarm with a manual reset.
 *
 * SAFETY: educational prototype only - see the SAFETY NOTE in
 * project_info.txt. Never rely on it instead of a certified smoke alarm.
 */

// ---------------- Named pin constants ----------------
const int SMOKE_DO_PIN = 2;        // MQ-2 digital (comparator) output
const int RESET_BUTTON_PIN = 3;    // press to clear a latched alarm
const int POWER_LED_PIN = 4;       // green: system armed
const int ALARM_LED_PIN = 6;       // red: alarm latched
const int BUZZER_PIN = 8;          // alarm sounder
const int SMOKE_AO_PIN = A0;       // MQ-2 analog output (extra monitoring)

// Most MQ-2 boards pull DO LOW when gas/smoke is detected. Set to false if
// your module does the opposite (you can test this by lighter gas near it).
const bool SMOKE_ACTIVE_LOW = true;

const unsigned long DEBOUNCE_MS = 40;    // button stability time
const unsigned long STATUS_MS = 1000;    // Serial status every second

// ---------------- State ----------------
bool alarmLatched = false;              // once true, only the reset button clears it
int lastRawReset = HIGH;
int lastStableReset = HIGH;
unsigned long lastResetChangeMs = 0;
unsigned long lastStatusMs = 0;

// Reads the MQ-2 digital pin and returns true when smoke/gas is present.
bool smokeDetected() {
  int raw = digitalRead(SMOKE_DO_PIN);
  return SMOKE_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

// Turn the alarm outputs on (latch) or off (reset).
void setAlarm(bool on) {
  alarmLatched = on;
  digitalWrite(ALARM_LED_PIN, on ? HIGH : LOW);

  if (on) {
    tone(BUZZER_PIN, 2000);   // continuous 2 kHz alarm tone
  } else {
    noTone(BUZZER_PIN);
  }
}

// Debounced reset button: returns true once per clean press.
bool resetPressed() {
  int raw = digitalRead(RESET_BUTTON_PIN);
  unsigned long now = millis();

  if (raw != lastRawReset) {              // bounce restarts the stability timer
    lastRawReset = raw;
    lastResetChangeMs = now;
  }
  if (now - lastResetChangeMs >= DEBOUNCE_MS && raw != lastStableReset) {
    lastStableReset = raw;
    if (raw == LOW) return true;          // pulled to GND = pressed
  }
  return false;
}

void setup() {
  Serial.begin(9600);

  pinMode(SMOKE_DO_PIN, INPUT);
  pinMode(SMOKE_AO_PIN, INPUT);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(POWER_LED_PIN, HIGH);   // green LED = armed
  setAlarm(false);                     // start with the alarm off

  Serial.println("Smoke Detection Alarm ready (MQ-2 on pin 2, A0 monitored).");
  Serial.println("Alarm LATCHES ON on detection - press the button on pin 3 to reset.");
  Serial.println("NOTE: new MQ-2 sensors need 24-48 hours of warm-up before readings settle.");
}

void loop() {
  bool detected = smokeDetected();
  int analogValue = analogRead(SMOKE_AO_PIN);   // 0..1023 raw concentration proxy

  // ---- Latch the alarm the first time smoke is seen ----
  if (detected && !alarmLatched) {
    setAlarm(true);
    Serial.print("*** SMOKE DETECTED - ALARM LATCHED ON at t=");
    Serial.print(millis() / 1000);
    Serial.println(" s. Press the reset button (pin 3) to clear. ***");
  }

  // ---- Manual reset ----
  if (resetPressed()) {
    if (alarmLatched) {
      setAlarm(false);
      Serial.println("Alarm RESET by button - monitoring again.");
    } else {
      Serial.println("Reset pressed - alarm was already clear.");
    }
  }

  // ---- Status line every second ----
  if (millis() - lastStatusMs >= STATUS_MS) {
    lastStatusMs = millis();
    Serial.print("Analog(A0): ");
    Serial.print(analogValue);
    Serial.print(" | Digital(pin 2): ");
    Serial.print(detected ? "SMOKE" : "clear");
    Serial.print(" | State: ");
    Serial.println(alarmLatched ? "ALARM (latched)" : "monitoring");
  }

  delay(10);   // gentle 100 Hz polling, the alarm itself is latched
}
