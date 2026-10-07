// 004 - Push Button Counter
// Press a push button and the Arduino counts each press, printing the
// running total to the Serial Monitor. The input is debounced so one
// physical press always adds exactly one to the count.
// Board: Arduino Uno
// Wiring: Push button between digital pin 2 and GND (internal pull-up is used,
//         so no external resistor is required).

// Pin and timing constants
const int BUTTON_PIN = 2;        // Button input pin
const unsigned long DEBOUNCE_MS = 25;  // Ignore bouncing shorter than 25 ms

// Variables that remember the previous state of the button
int lastReading = HIGH;          // INPUT_PULLUP reads HIGH when the button is open
int stableState = HIGH;          // The debounced (settled) state
unsigned long lastChangeTime = 0; // When the state last started changing

int pressCount = 0;              // How many presses we have counted

void setup() {
  // INPUT_PULLUP enables the internal pull-up resistor, so the pin reads
  // HIGH when the button is open and LOW when it is pressed to GND.
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Push Button Counter ready. Press the button!");
}

void loop() {
  // Read the raw button level right now
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // If the raw reading changed, restart the debounce timer
  if (reading != lastReading) {
    lastChangeTime = now;
  }

  // Accept the new reading only if it has stayed stable for DEBOUNCE_MS
  if ((now - lastChangeTime) >= DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;

      // The button closes the circuit: HIGH -> LOW means a new press
      if (stableState == LOW) {
        pressCount++;
        Serial.print("Button pressed! Count = ");
        Serial.println(pressCount);
      }
    }
  }

  // Remember the raw reading for the next pass through loop()
  lastReading = reading;
}
