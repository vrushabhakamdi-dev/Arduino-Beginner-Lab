// 016 - Digital Stopwatch
// Board: Arduino Uno
// Wiring:
//   Start/Stop button -> one leg to pin 2, other leg to GND (internal pull-up)
//   Reset button      -> one leg to pin 3, other leg to GND (internal pull-up)
// Open the Serial Monitor at 9600 baud. The time is shown as mm:ss:cs
// (minutes : seconds : hundredths of a second).
// loop() never calls delay(), so the buttons always react immediately.

const int START_BUTTON_PIN = 2;   // Press to start or stop the stopwatch
const int RESET_BUTTON_PIN = 3;   // Press to clear the time back to zero
const unsigned long DEBOUNCE_MS = 30;  // A press must stay stable this long
const unsigned long DISPLAY_MS = 20;   // How often the display line is refreshed

// ---------------- Stopwatch state ----------------
bool running = false;
unsigned long savedMs = 0;         // Time accumulated before the last start
unsigned long runStartMillis = 0;  // millis() captured when the stopwatch started

// ---------------- Debounce bookkeeping (one block per button) ----------------
int startRaw = HIGH, startStable = HIGH;
unsigned long startLastChange = 0;
int resetRaw = HIGH, resetStable = HIGH;
unsigned long resetLastChange = 0;

unsigned long lastDisplay = 0;     // Last time the display line was refreshed

// Returns true exactly once each time a button is pressed.
// The reading must stay stable for DEBOUNCE_MS before the press is accepted.
bool buttonPressed(int pin, int &raw, int &stable, unsigned long &lastChange) {
  int reading = digitalRead(pin);
  if (reading != raw) {            // Contact bounced, restart the stability timer
    raw = reading;
    lastChange = millis();
  }
  if (raw != stable && (millis() - lastChange) >= DEBOUNCE_MS) {
    stable = raw;
    if (stable == LOW) return true;  // LOW means pressed (INPUT_PULLUP is used)
  }
  return false;
}

// Total time in milliseconds, including the part measured before the last stop
unsigned long totalTime() {
  if (running) return savedMs + (millis() - runStartMillis);
  return savedMs;
}

// Prints mm:ss:cs for a given number of milliseconds
void printTime(unsigned long total) {
  unsigned long minutes = (total / 60000UL) % 100;
  unsigned long seconds = (total / 1000UL) % 60;
  unsigned long centis = (total % 1000UL) / 10UL;  // Hundredths of a second

  if (minutes < 10) Serial.print('0');
  Serial.print(minutes);
  Serial.print(':');
  if (seconds < 10) Serial.print('0');
  Serial.print(seconds);
  Serial.print(':');
  if (centis < 10) Serial.print('0');
  Serial.print(centis);
}

void setup() {
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("=== Digital Stopwatch ===");
  Serial.println("START/STOP = pin 2, RESET = pin 3");
  Serial.println("00:00:00");
}

void loop() {
  // ---------------- Start / Stop button ----------------
  if (buttonPressed(START_BUTTON_PIN, startRaw, startStable, startLastChange)) {
    if (running) {
      savedMs = totalTime();   // Freeze the elapsed time at this moment
      running = false;
      Serial.println();
      Serial.print("STOPPED at ");
      printTime(savedMs);
      Serial.println();
    } else {
      runStartMillis = millis();  // Continue from the frozen value
      running = true;
      Serial.println("STARTED");
    }
  }

  // ---------------- Reset button ----------------
  if (buttonPressed(RESET_BUTTON_PIN, resetRaw, resetStable, resetLastChange)) {
    running = false;
    savedMs = 0;
    Serial.println();
    Serial.println("RESET -> 00:00:00");
  }

  // ---------------- Live display (non-blocking) ----------------
  if (running && (millis() - lastDisplay) >= DISPLAY_MS) {
    lastDisplay = millis();
    Serial.print('\r');           // Carriage return moves the cursor back to the
    printTime(totalTime());       // start of the line, so the same line is updated
  }
}
