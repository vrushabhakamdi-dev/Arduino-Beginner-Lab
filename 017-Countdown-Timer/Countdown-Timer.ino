// 017 - Countdown Timer
// Board: Arduino Uno
// Wiring:
//   Potentiometer: one outer pin -> 5V, other outer pin -> GND, wiper (middle) -> A0
//   Start button  -> one leg to pin 2, other leg to GND (internal pull-up)
//   LED           -> pin 13 through a 220 ohm resistor
//   Passive buzzer -> (+) to pin 8, (-) to GND
// Turn the pot to choose 10-120 seconds, press the button to start, watch the
// countdown in the Serial Monitor, and listen for three beeps when time is up.

const int POT_PIN = A0;           // Potentiometer selects the countdown length
const int BUTTON_PIN = 2;         // Start (and cancel) button
const int LED_PIN = 13;           // Status LED
const int BUZZER_PIN = 8;         // Passive buzzer for the alarm

// ---------------- Time settings ----------------
const int MIN_SECONDS = 10;       // Shortest countdown
const int MAX_SECONDS = 120;      // Longest countdown
const int BLINK_LAST_SECONDS = 5; // The LED blinks during the final 5 seconds
const unsigned long BLINK_MS = 250;   // Half of one LED blink period

// ---------------- Alarm settings ----------------
const unsigned int BEEP_HZ = 1000;     // Pitch of the alarm
const unsigned int BEEP_MS = 200;      // Length of each beep
const unsigned int BEEP_GAP_MS = 200;  // Silence between beeps
const int BEEP_COUNT = 3;              // Three beeps when the time runs out

const unsigned long DEBOUNCE_MS = 30;  // Button press must stay stable this long

// The sketch is always in one of these two states
enum TimerState { STATE_SETTING, STATE_RUNNING };
TimerState state = STATE_SETTING;

int selectedSeconds = MIN_SECONDS;  // Value currently chosen by the potentiometer
int lastShownSelection = -1;        // Prints the selection only when it changes
int totalSeconds = 0;               // Countdown length frozen when the button is pressed
int lastShownCountdown = -1;        // Prints each remaining second only once
unsigned long runStartMillis = 0;   // millis() captured when the countdown started
unsigned long lastBlinkMillis = 0;  // Last time the LED changed state
bool ledOn = false;

// Debounce bookkeeping for the button
int buttonRaw = HIGH, buttonStable = HIGH;
unsigned long buttonLastChange = 0;

// Returns true once each time the button is pressed
bool startPressed() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != buttonRaw) {
    buttonRaw = reading;
    buttonLastChange = millis();
  }
  if (buttonRaw != buttonStable && (millis() - buttonLastChange) >= DEBOUNCE_MS) {
    buttonStable = buttonRaw;
    if (buttonStable == LOW) return true;
  }
  return false;
}

// Reads the potentiometer and waits for the button that starts the countdown
void handleSetting() {
  int value = analogRead(POT_PIN);
  selectedSeconds = map(value, 0, 1023, MIN_SECONDS, MAX_SECONDS);

  if (selectedSeconds != lastShownSelection) {
    lastShownSelection = selectedSeconds;
    Serial.print("Selected time: ");
    Serial.print(selectedSeconds);
    Serial.println(" s   (turn the potentiometer, press the button to start)");
  }

  if (startPressed()) {
    totalSeconds = selectedSeconds;      // Freeze the chosen value
    runStartMillis = millis();
    lastShownCountdown = -1;
    lastBlinkMillis = millis();
    ledOn = false;
    digitalWrite(LED_PIN, LOW);
    state = STATE_RUNNING;
    Serial.print("COUNTDOWN STARTED: ");
    Serial.print(totalSeconds);
    Serial.println(" s");
  }
}

// Counts the seconds down, blinks the LED at the end and sounds the alarm
void handleRunning() {
  unsigned long elapsed = millis() - runStartMillis;
  long remaining = (long)totalSeconds - (long)(elapsed / 1000UL);

  if (remaining <= 0) {
    // Time is up: switch the LED off and play three short beeps.
    // delay() is acceptable here because the countdown has already finished.
    digitalWrite(LED_PIN, LOW);
    for (int i = 0; i < BEEP_COUNT; i++) {
      tone(BUZZER_PIN, BEEP_HZ);
      delay(BEEP_MS);
      noTone(BUZZER_PIN);
      if (i < BEEP_COUNT - 1) delay(BEEP_GAP_MS);
    }
    Serial.println("TIME'S UP!");
    Serial.println();
    state = STATE_SETTING;          // Go back to the setup screen
    lastShownSelection = -1;
    return;
  }

  // Print every remaining second exactly once
  if (remaining != lastShownCountdown) {
    lastShownCountdown = remaining;
    Serial.print("T-");
    Serial.print(remaining);
    Serial.println(" s");
  }

  // LED stays on while counting, and blinks during the final seconds
  if (remaining <= BLINK_LAST_SECONDS) {
    if (millis() - lastBlinkMillis >= BLINK_MS) {
      lastBlinkMillis = millis();
      ledOn = !ledOn;
      digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
    }
  } else {
    if (!ledOn) {
      ledOn = true;
      digitalWrite(LED_PIN, HIGH);
    }
  }

  // Pressing the button again cancels the countdown and returns to setup
  if (startPressed()) {
    digitalWrite(LED_PIN, LOW);
    ledOn = false;
    Serial.println("CANCELLED - choose a new time.");
    state = STATE_SETTING;
    lastShownSelection = -1;
  }
}

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("=== Countdown Timer ===");
  Serial.println("Turn the potentiometer to choose 10-120 seconds, then press the button.");
}

void loop() {
  // Both states are checked with millis(), so nothing blocks the loop
  if (state == STATE_SETTING) {
    handleSetting();
  } else {
    handleRunning();
  }
}
