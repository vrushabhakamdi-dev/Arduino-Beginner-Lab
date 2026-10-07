// 033 - Magnetic Door Detector
// Uses a reed switch + magnet to know whether a door or window is OPEN or CLOSED.
// Board: Arduino Uno
// Wiring:
//   Reed switch one leg -> pin 2, other leg -> GND
//   (pin 2 is set to INPUT_PULLUP, so NO external resistor is needed)
//   Built-in LED on pin 13 lights up when the door is OPEN
//
// How the reed switch behaves:
//   Magnet CLOSE to the switch  -> switch closes -> pin 2 pulled to GND -> reads LOW
//   Magnet FAR from the switch  -> switch opens  -> pin 2 floats HIGH  -> reads HIGH

// ---- Pin constants ----
const int REED_PIN = 2;   // Reed switch input, uses the internal pull-up resistor
const int LED_PIN = 13;   // Built-in LED: ON means the door is OPEN

// ---- Timing constants ----
const unsigned long DEBOUNCE_MS = 50;  // Reed contacts bounce for a few ms

int lastState = -1;               // -1 means "unknown", forces a first message
int candidateState = -1;          // Reading we are currently waiting to confirm
unsigned long candidateTime = 0;  // When that candidate reading first appeared

void setup() {
  // INPUT_PULLUP enables a resistor inside the chip that holds pin 2 at 5V.
  // When the reed switch closes it connects the pin straight to GND, reading LOW.
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Magnetic Door Detector starting...");

  // Read once so we can print the starting condition right away
  lastState = digitalRead(REED_PIN);
  printState(lastState, "startup");
}

void loop() {
  int currentState = digitalRead(REED_PIN);

  // ---- Debounce: reed contacts physically bounce for a few milliseconds.
  //      We accept a new state only after it has stayed the same for DEBOUNCE_MS. ----
  if (currentState != lastState) {

    // First time we see this different reading: start the waiting timer
    if (currentState != candidateState) {
      candidateState = currentState;
      candidateTime = millis();
    }
    // Same different reading held steady long enough: believe it
    else if (millis() - candidateTime >= DEBOUNCE_MS) {
      lastState = currentState;
      printState(currentState, "change");
    }
  } else {
    // Reading matches the confirmed state, so cancel any pending candidate
    candidateState = currentState;
  }
}

// Prints a message ONLY on a state change, including an absolute millisecond timestamp.
void printState(int state, const char* reason) {
  // LOW  = magnet is close = door CLOSED
  // HIGH = magnet is away   = door OPEN
  bool doorOpen = (state == HIGH);

  // LED shows the OPEN condition so you can read the status from across the room
  digitalWrite(LED_PIN, doorOpen ? HIGH : LOW);

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");
  Serial.print(reason);
  Serial.print(" -> ");
  Serial.println(doorOpen ? "DOOR OPEN" : "DOOR CLOSED");
}
