// 090 - Laser Tripwire Alarm
// A laser beam shines across a doorway onto an LDR. As soon as the beam is
// broken the alarm latches on - buzzer and LED stay active until the reset
// button is pressed.
// Board: Arduino Uno
//
// LASER SAFETY - READ THIS FIRST:
//   * Never point the laser at anyone's eyes, human or animal.
//   * Never use a laser pointer or a laser above class 2 (< 1 mW) for this
//     project, and mount the beam at ankle or knee height across the doorway.
//   * Do not look directly into the beam while aligning it - aim it at the
//     LDR and check the alignment by watching the Serial readings instead.
//
// Wiring:
//   Laser module: + -> digital pin 7, - -> GND
//     (use a module with a built-in driver resistor; a bare diode needs one)
//   LDR voltage divider: 5V -> LDR -> analog pin A0 -> 10k ohm -> GND
//     More light on the LDR = higher reading (0-1023), so the alarm trips
//     when the reading FALLS below the calibrated threshold.
//   Piezo buzzer: (+) -> digital pin 8, (-) -> GND
//   Alarm LED: digital pin 13 -> 220 ohm -> LED -> GND
//     (the Uno's built-in LED on pin 13 works, no extra parts needed)
//   Reset push button: one leg -> digital pin 2, other leg -> GND
//     (pin 2 uses INPUT_PULLUP, so no external resistor is needed)
//
// CALIBRATION:
//   On start-up the laser is switched on, given half a second to settle and
//   then 100 readings are averaged to produce the BASELINE - that is the
//   value with the beam safely landing on the LDR. The threshold is set
//   DROP_PERCENT (25%) below that baseline, so blocking the beam makes the
//   reading collapse and triggers the alarm. Ambient light changes the
//   baseline, so press RESET while the alarm is silent (or send 'c' over
//   Serial) to calibrate again. Run calibration with the beam intact, i.e.
//   with nobody walking through the doorway.
//
// SAFETY: see the SAFETY NOTE in project_info.txt - this is a classroom
// prototype and must never be used as a real security or safety system.

// ---------------- Pin constants ----------------
const int LDR_PIN = A0;       // LDR divider analog input
const int LASER_PIN = 7;      // laser module power enable
const int BUZZER_PIN = 8;     // piezo alarm
const int LED_PIN = 13;       // alarm LED (on-board LED works)
const int RESET_PIN = 2;      // reset / recalibrate button (to GND)

// ---------------- Behaviour settings ----------------
const int DROP_PERCENT = 25;        // trip when reading falls this far below baseline
const int CALIBRATION_SAMPLES = 100;
const unsigned long CALIBRATION_DELAY_MS = 10;  // 100 samples x 10 ms = 1 second
const unsigned long LASER_SETTLE_MS = 500;      // warm-up time for the laser
const unsigned long DEBOUNCE_MS = 250;          // button debounce
const unsigned long PRINT_MS = 500;             // Serial heartbeat interval
const int ALARM_HZ = 1000;                      // continuous alarm pitch
const int BUTTON_PRESSED = LOW;                 // INPUT_PULLUP reads LOW when pressed
const int MIN_THRESHOLD = 10;                   // never aim the threshold at zero

// ---------------- State variables ----------------
int baseline = 0;        // average reading with the beam intact
int threshold = 0;       // reading below this = beam broken
bool tripped = false;    // latched alarm flag
bool resetWasDown = false;
unsigned long lastResetMs = 0;
unsigned long lastPrintMs = 0;

// Average 100 readings with the beam intact and set the trip threshold
void calibrate() {
  Serial.println("Calibrating: keep the beam on the LDR for 1 second...");

  digitalWrite(LASER_PIN, HIGH);
  delay(LASER_SETTLE_MS);   // let the laser brightness stabilise first

  long total = 0;
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    total += analogRead(LDR_PIN);
    delay(CALIBRATION_DELAY_MS);
  }
  baseline = (int)(total / CALIBRATION_SAMPLES);

  threshold = baseline - (baseline * DROP_PERCENT) / 100;
  if (threshold < MIN_THRESHOLD) threshold = MIN_THRESHOLD;

  Serial.print("Baseline: ");
  Serial.print(baseline);
  Serial.print(" | Threshold: ");
  Serial.println(threshold);
  Serial.println("Calibration done - watching the beam.");
}

void tripAlarm() {
  tripped = true;
  tone(BUZZER_PIN, ALARM_HZ);      // continuous tone, it latches on
  digitalWrite(LED_PIN, HIGH);
  Serial.println("BEAM BROKEN -> ALARM (press RESET to silence)");
}

void resetAlarm() {
  tripped = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  Serial.println("ALARM RESET - watching the beam again.");
}

void setup() {
  pinMode(LDR_PIN, INPUT);
  pinMode(LASER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(LASER_PIN, HIGH);   // beam on for the whole session

  Serial.begin(9600);
  Serial.println("090 - Laser Tripwire Alarm ready.");
  Serial.println("Send 'c' over Serial to recalibrate (only when not tripped).");

  calibrate();
}

void loop() {
  unsigned long now = millis();
  int reading = analogRead(LDR_PIN);

  // ----- 1. Trip when the reading falls below the threshold -----
  if (!tripped && reading < threshold) {
    tripAlarm();
  }

  // ----- 2. Reset / recalibrate button (debounced edge) -----
  bool resetDown = (digitalRead(RESET_PIN) == BUTTON_PRESSED);
  if (resetDown && !resetWasDown && (now - lastResetMs) >= DEBOUNCE_MS) {
    lastResetMs = now;
    resetWasDown = true;

    if (tripped) {
      resetAlarm();
    } else {
      calibrate();   // alarm is quiet, so take a fresh baseline
    }
  }
  if (!resetDown) resetWasDown = false;

  // ----- 3. Optional recalibrate command from the Serial Monitor -----
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') {
      if (tripped) {
        Serial.println("Alarm is sounding - press RESET first.");
      } else {
        calibrate();
      }
    }
  }

  // ----- 4. Readings every 500 ms so the alignment can be tuned -----
  if (now - lastPrintMs >= PRINT_MS) {
    lastPrintMs = now;

    Serial.print("LDR: ");
    Serial.print(reading);
    Serial.print(" | baseline: ");
    Serial.print(baseline);
    Serial.print(" | threshold: ");
    Serial.print(threshold);
    Serial.print(" | Beam: ");
    Serial.print(reading < threshold ? "BROKEN" : "intact");
    Serial.print(" | Alarm: ");
    Serial.println(tripped ? "TRIPPED" : "off");
  }

  delay(20);
}
