// 089 - Motion Security Alarm
// A PIR sensor watches a room. Pressing the button arms the system, a green
// LED shows it is armed, and any motion while armed starts a siren on the
// buzzer plus a flashing LED until the button is pressed again.
// Board: Arduino Uno
//
// Wiring:
//   HC-SR501 PIR module: OUT -> digital pin 2, VCC -> 5V, GND -> GND
//     (set the module's delay pot to minimum and the mode jumper to H = repeat)
//   Arm / disarm push button: one leg -> digital pin 3, other leg -> GND
//     (pin 3 uses INPUT_PULLUP, so no external resistor is needed)
//   Armed LED: digital pin 12 -> 220 ohm -> LED -> GND
//   Alarm LED: digital pin 13 -> 220 ohm -> LED -> GND (on-board LED works too)
//   Piezo buzzer: (+) -> digital pin 8, (-) -> GND
//
// START-UP: a PIR needs 30-60 seconds to settle after power-up. The system
// therefore boots DISARMED, so you have time to leave the room and press the
// arm button once the sensor has stabilised.
//
// SAFETY: this is an educational prototype. See the SAFETY NOTE in
// project_info.txt - never wire it to anything that could harm a person or
// an animal.

// ---------------- Pin constants ----------------
const int PIR_PIN = 2;        // PIR motion output
const int ARM_BUTTON_PIN = 3; // arm / disarm button (to GND)
const int ARMED_LED_PIN = 12; // stays ON while the system is armed
const int ALARM_LED_PIN = 13; // flashes while the siren sounds
const int BUZZER_PIN = 8;     // piezo siren

// ---------------- Behaviour settings ----------------
const int BUTTON_PRESSED = LOW;          // INPUT_PULLUP reads LOW when pressed
const unsigned long DEBOUNCE_MS = 200;   // ignore button chatter
const unsigned long SIREN_INTERVAL_MS = 150;  // siren pitch flip period
const unsigned long MOTION_CONFIRM_MS = 60;   // PIR must stay HIGH this long
const unsigned long STATUS_MS = 5000;    // Serial heartbeat interval
const int SIREN_LOW_HZ = 800;            // low siren pitch
const int SIREN_HIGH_HZ = 1200;          // high siren pitch

// ---------------- State machine ----------------
enum SystemState { DISARMED, ARMED, ALARM };
SystemState state = DISARMED;

unsigned long lastButtonMs = 0;      // debounce timer
unsigned long stateStartMs = 0;      // when the current state began
unsigned long motionSinceMs = 0;     // when motion was first seen (0 = none)
unsigned long sirenFlipMs = 0;       // last siren pitch change
bool sirenHigh = false;              // which pitch is playing now
bool buttonWasPressed = false;       // previous debounced button level
unsigned long lastStatusMs = 0;

// Human readable state names for the Serial log
const char* stateName(SystemState s) {
  switch (s) {
    case DISARMED: return "DISARMED";
    case ARMED:    return "ARMED";
    case ALARM:    return "ALARM";
  }
  return "?";
}

// Print an event with a seconds-since-start timestamp
void logEvent(const char* message) {
  unsigned long totalSeconds = millis() / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  char stamp[12];
  snprintf(stamp, sizeof(stamp), "%02lu:%02lu:%02lu", hours, minutes, seconds);

  Serial.print('[');
  Serial.print(stamp);
  Serial.print("] ");
  Serial.println(message);
}

// True exactly once per fresh button press
bool buttonPressed() {
  bool down = (digitalRead(ARM_BUTTON_PIN) == BUTTON_PRESSED);

  if (down && !buttonWasPressed && (millis() - lastButtonMs) >= DEBOUNCE_MS) {
    lastButtonMs = millis();
    buttonWasPressed = true;
    return true;
  }
  if (!down) {
    buttonWasPressed = false;
  }
  return false;
}

// Move to a new state, update the LEDs and write the transition to Serial
void enterState(SystemState next) {
  state = next;
  stateStartMs = millis();

  digitalWrite(ARMED_LED_PIN, (next == ARMED || next == ALARM) ? HIGH : LOW);
  digitalWrite(ALARM_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  if (next == ALARM) {
    // Restart the siren at the low pitch
    sirenFlipMs = millis();
    sirenHigh = false;
    tone(BUZZER_PIN, SIREN_LOW_HZ);
    digitalWrite(ALARM_LED_PIN, HIGH);
  }

  logEvent(stateName(next));
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(ARM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ARMED_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(ARMED_LED_PIN, LOW);
  digitalWrite(ALARM_LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("089 - Motion Security Alarm ready.");
  Serial.println("Press the button on pin 3 to arm, press again to disarm.");
  Serial.println("Let the PIR settle for 30-60 seconds before the first arm.");

  logEvent("DISARMED");
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Arm / disarm button -----
  if (buttonPressed()) {
    if (state == DISARMED) {
      motionSinceMs = 0;              // ignore motion seen while disarming
      enterState(ARMED);
      Serial.println("System armed - motion will trigger the siren.");
    } else {
      // ARMED or ALARM -> back to DISARMED (this also silences the siren)
      motionSinceMs = 0;
      enterState(DISARMED);
      Serial.println("System disarmed - siren off.");
    }
  }

  // ----- 2. Motion handling -----
  if (state == ARMED || state == ALARM) {
    bool motionNow = (digitalRead(PIR_PIN) == HIGH);

    // Require the PIR to stay HIGH for 60 ms so a single noisy pulse
    // cannot set off a false alarm.
    if (motionNow) {
      if (motionSinceMs == 0) {
        motionSinceMs = now;
      } else if (now - motionSinceMs >= MOTION_CONFIRM_MS && state == ARMED) {
        enterState(ALARM);
        Serial.println("MOTION DETECTED while armed!");
      }
    } else {
      motionSinceMs = 0;
    }
  }

  // ----- 3. Siren and flashing alarm LED (non blocking) -----
  if (state == ALARM) {
    if (now - sirenFlipMs >= SIREN_INTERVAL_MS) {
      sirenFlipMs = now;
      sirenHigh = !sirenHigh;
      tone(BUZZER_PIN, sirenHigh ? SIREN_HIGH_HZ : SIREN_LOW_HZ);
      digitalWrite(ALARM_LED_PIN, sirenHigh ? HIGH : LOW);
    }
  }

  // ----- 4. Status line every 5 seconds -----
  if (now - lastStatusMs >= STATUS_MS) {
    lastStatusMs = now;
    Serial.print("State: ");
    Serial.print(stateName(state));
    Serial.print(" | PIR: ");
    Serial.print(digitalRead(PIR_PIN) == HIGH ? "MOTION" : "clear");
    Serial.print(" | Armed LED: ");
    Serial.print(digitalRead(ARMED_LED_PIN) == HIGH ? "ON" : "off");
    Serial.println();
  }

  delay(10);
}
