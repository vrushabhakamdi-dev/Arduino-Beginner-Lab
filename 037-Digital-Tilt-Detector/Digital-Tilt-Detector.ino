// 037 - Digital Tilt Detector
// Detects when a board is tipped over using an SW-520D tilt ball switch.
// Board: Arduino Uno
// Wiring:
//   SW-520D module  ->  VCC = 5V, GND = GND, DO = pin 2
//   Buzzer (+)      ->  pin 8 through a 220 ohm resistor, buzzer (-) = GND
//   Built-in LED    ->  pin 13 lights up when the switch reports a tilt
//
// How the SW-520D works:
//   It is a small metal cylinder with a loose ball inside. When the module is
//   roughly level the ball rests on one contact and the output stays LOW.
//   Tip it and the ball rolls onto the other contact, driving the output HIGH.

// ---- Pin constants ----
const int TILT_PIN = 2;   // Digital output of the tilt sensor
const int BUZZER_PIN = 8; // Piezo buzzer that sounds while tilted
const int LED_PIN = 13;   // Built-in LED indicator

// ---- Timing constants ----
const unsigned long DEBOUNCE_MS = 50;    // The rolling ball can rattle a little
const unsigned long REPORT_MS = 500;     // Status line interval while tilted

int lastState = -1;                // -1 forces the first reading to be printed
int candidateState = -1;           // Reading waiting to be confirmed
unsigned long candidateTime = 0;   // When that candidate reading first appeared
unsigned long lastReport = 0;      // Last status line while tilted
unsigned long tiltStart = 0;       // When the current tilt began

void setup() {
  pinMode(TILT_PIN, INPUT);   // The module drives this pin
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Digital Tilt Detector starting...");
  Serial.println("Lay the board flat, then tip it to trigger the alarm.");

  // Read once so the starting condition is shown immediately
  lastState = digitalRead(TILT_PIN);
  reportState(lastState, "startup");
}

void loop() {
  int currentState = digitalRead(TILT_PIN);

  // ---- Debounce: wait until the reading stays the same for DEBOUNCE_MS ----
  if (currentState != lastState) {
    if (currentState != candidateState) {
      candidateState = currentState;
      candidateTime = millis();       // Start the confirmation timer
    } else if (millis() - candidateTime >= DEBOUNCE_MS) {
      lastState = currentState;
      reportState(currentState, "change");
    }
  } else {
    candidateState = currentState;    // Reading settled, cancel the timer
  }

  // ---- While tilted, keep the alarm sounding and report how long it lasts ----
  if (lastState == HIGH) {
    tone(BUZZER_PIN, 800);   // Low, steady tone while on its side

    if (millis() - lastReport >= REPORT_MS) {
      lastReport = millis();
      Serial.print("[");
      Serial.print(millis() / 1000);
      Serial.print(" s] tilted for ");
      Serial.print(millis() - tiltStart);
      Serial.println(" ms");
    }
  } else {
    noTone(BUZZER_PIN);      // Quiet while the board is level
  }
}

// Prints TILTED or LEVEL only when the state actually changes.
void reportState(int state, const char* reason) {
  bool tilted = (state == HIGH);

  digitalWrite(LED_PIN, tilted ? HIGH : LOW);

  if (tilted) {
    tiltStart = millis();   // Remember when the tilt began
    lastReport = millis();
  }

  Serial.print("[");
  Serial.print(millis() / 1000);
  Serial.print(" s] ");
  Serial.print(reason);
  Serial.print(" -> ");
  Serial.println(tilted ? "TILTED" : "LEVEL");
}
