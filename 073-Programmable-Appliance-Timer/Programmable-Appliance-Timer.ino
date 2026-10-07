// 073 - Programmable Appliance Timer
// Pick a time of 1 to 60 minutes with a potentiometer, press a button to start,
// and a relay keeps an "appliance" switched on until the countdown reaches zero.
// Board: Arduino Uno
// Wiring:
//   Potentiometer: outer pins -> 5V and GND, wiper -> pin A0
//   Start/cancel button: pin 2 -> GND (INPUT_PULLUP is used)
//   Progress LEDs: pins 4, 5, 6 and 7, each through a 220 ohm resistor to GND
//   Relay module IN: pin 9 (switches the demo appliance)
//   Passive buzzer: pin 8
//
// SAFETY: The relay switches a low-voltage demo load only. Never connect mains
// electricity to a breadboard prototype.

// ---------------- Pin constants ----------------
const int POT_PIN = A0;                 // Potentiometer selects the time
const int BUTTON_PIN = 2;               // Start / cancel button
const int LED_PINS[4] = {4, 5, 6, 7};   // Quarter progress LEDs
const int BUZZER_PIN = 8;               // Buzzer sounds when time is up
const int RELAY_PIN = 9;                // Relay turns the appliance on/off

// ---------------- Timing constants ----------------
const unsigned long DEBOUNCE_MS = 50;   // Button debounce time
const int MIN_MINUTES = 1;              // Shortest selectable time
const int MAX_MINUTES = 60;             // Longest selectable time

// ---------------- State variables ----------------
bool timerRunning = false;
int selectedMinutes = 5;        // Time chosen with the potentiometer
unsigned long startMs = 0;      // millis() when the countdown started
unsigned long durationMs = 0;   // Total countdown length in milliseconds
unsigned long lastSecondMs = 0; // Last time we printed / updated one second
int lastPrintedSeconds = -1;    // Used to print each countdown second only once

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceMs = 0;

// Print the remaining time as mm:ss
void printCountdown(unsigned long remainingMs) {
  unsigned long totalSeconds = (remainingMs + 999) / 1000;  // Round up
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  Serial.print("Countdown: ");
  if (minutes < 10) Serial.print('0');
  Serial.print(minutes);
  Serial.print(':');
  if (seconds < 10) Serial.print('0');
  Serial.println(seconds);
}

// Light one LED per completed quarter (cumulative fill)
void showProgress(unsigned long elapsedMs) {
  int quartersDone = (elapsedMs * 4UL) / durationMs;  // 0..4
  if (quartersDone > 4) quartersDone = 4;

  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], (i < quartersDone) ? HIGH : LOW);
  }
}

// Switch every output off and clear the progress LEDs
void stopEverything() {
  digitalWrite(RELAY_PIN, LOW);  // Appliance OFF (most relay modules: LOW = off)
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
  stopEverything();  // Make sure nothing is switched on at boot

  Serial.begin(9600);
  Serial.println("Programmable Appliance Timer ready.");
  Serial.println("Turn the pot to set 1-60 minutes, then press the button.");
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Read the potentiometer (1..60 minutes) while idle -----
  if (!timerRunning) {
    int potValue = analogRead(POT_PIN);  // 0..1023
    int newMinutes = map(potValue, 0, 1023, MIN_MINUTES, MAX_MINUTES);
    if (newMinutes != selectedMinutes) {
      selectedMinutes = newMinutes;
      Serial.print("Set time: ");
      Serial.print(selectedMinutes);
      Serial.println(" min");
    }
  }

  // ----- 2. Read and debounce the start / cancel button -----
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonReading) {
    lastDebounceMs = now;
  }
  lastButtonReading = reading;

  if ((now - lastDebounceMs) > DEBOUNCE_MS && reading != stableButtonState) {
    stableButtonState = reading;

    if (stableButtonState == LOW) {  // Pressed
      if (!timerRunning) {
        // ----- Start the countdown -----
        timerRunning = true;
        startMs = now;
        durationMs = (unsigned long)selectedMinutes * 60000UL;
        lastSecondMs = now;
        lastPrintedSeconds = -1;

        digitalWrite(RELAY_PIN, HIGH);  // Appliance ON
        Serial.print("TIMER STARTED - appliance ON for ");
        Serial.print(selectedMinutes);
        Serial.println(" min");
      } else {
        // ----- Cancel the countdown -----
        timerRunning = false;
        stopEverything();  // Appliance OFF
        Serial.println("TIMER CANCELLED - appliance OFF");
      }
    }
  }

  // ----- 3. Update the running countdown once per second -----
  if (timerRunning) {
    unsigned long elapsedMs = now - startMs;

    if (elapsedMs >= durationMs) {
      // ----- Time is up -----
      timerRunning = false;
      stopEverything();
      Serial.println("TIME UP - appliance OFF");

      // Three short beeps as the alarm
      for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 2000);   // 2 kHz tone
        delay(150);
        noTone(BUZZER_PIN);
        delay(150);
      }
      Serial.println("Ready for the next time.");
    } else if (now - lastSecondMs >= 1000) {
      // ----- One second has passed: refresh LEDs and Serial -----
      lastSecondMs = now;
      showProgress(elapsedMs);

      unsigned long remainingMs = durationMs - elapsedMs;
      int remainingSeconds = remainingMs / 1000;
      if (remainingSeconds != lastPrintedSeconds) {
        lastPrintedSeconds = remainingSeconds;
        printCountdown(remainingMs);
      }
    }
  }

  delay(10);  // Keep the loop steady
}
