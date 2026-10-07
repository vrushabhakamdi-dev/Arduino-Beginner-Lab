// 072 - Automatic Curtain Prototype
// An LDR measures the room brightness and a servo motor opens or closes a model curtain.
// A push button gives a manual override of the automatic behaviour.
// Board: Arduino Uno
// Wiring:
//   LDR voltage divider: 5V -> LDR -> pin A0 -> 10k ohm resistor -> GND
//   Servo signal: pin 6 (power the servo from 5V, common GND with the Arduino)
//   Manual override button: pin 2 -> GND (INPUT_PULLUP is used)
//
// Libraries: Servo (built-in with the Arduino IDE)

#include <Servo.h>  // Built-in Arduino library for RC servo motors

// ---------------- Pin constants ----------------
const int LDR_PIN = A0;       // Analog pin reading the LDR voltage divider
const int SERVO_PIN = 6;      // Servo signal wire
const int BUTTON_PIN = 2;     // Manual override button

// ---------------- Brightness thresholds (hysteresis) ----------------
// Two different thresholds prevent the servo from jittering around one value:
// it opens above BRIGHT_THRESHOLD and only closes again below DARK_THRESHOLD.
const int BRIGHT_THRESHOLD = 600;  // Bright room -> open the curtain
const int DARK_THRESHOLD = 400;    // Dark room  -> close the curtain

// ---------------- Servo positions ----------------
const int CURTAIN_CLOSED_ANGLE = 0;   // Curtain fully closed
const int CURTAIN_OPEN_ANGLE = 90;    // Curtain fully open

const unsigned long DEBOUNCE_MS = 50;  // Button debounce time

// ---------------- State variables ----------------
Servo curtainServo;
bool curtainIsOpen = false;      // Current curtain position
bool manualMode = false;         // true = button controls the curtain, false = LDR controls it

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceMs = 0;

// Move the servo to the requested position and remember it
void setCurtain(bool open) {
  curtainIsOpen = open;
  curtainServo.write(open ? CURTAIN_OPEN_ANGLE : CURTAIN_CLOSED_ANGLE);
}

// Print the current curtain state to the Serial Monitor
void printState(const char *reason) {
  Serial.print(reason);
  Serial.print(" | Light: ");
  Serial.print(analogRead(LDR_PIN));
  Serial.print(" | Curtain: ");
  Serial.println(curtainIsOpen ? "OPEN" : "CLOSED");
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  curtainServo.attach(SERVO_PIN);
  setCurtain(false);  // Start with the curtain closed

  Serial.begin(9600);
  Serial.println("Automatic Curtain Prototype ready.");
  Serial.println("Bright >600 opens, dark <400 closes, button toggles manual mode.");
}

void loop() {
  // ----- 1. Read the light level -----
  int lightValue = analogRead(LDR_PIN);  // 0 (dark) ... 1023 (full light)
  bool isBright = (lightValue > BRIGHT_THRESHOLD);
  bool isDark = (lightValue < DARK_THRESHOLD);

  // ----- 2. Read and debounce the manual override button -----
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonReading) {
    lastDebounceMs = millis();
  }
  lastButtonReading = reading;

  if ((millis() - lastDebounceMs) > DEBOUNCE_MS && reading != stableButtonState) {
    stableButtonState = reading;

    if (stableButtonState == LOW) {  // Button pressed (INPUT_PULLUP -> LOW)
      manualMode = !manualMode;      // Switch between manual and automatic control

      if (manualMode) {
        // In manual mode the first press simply flips the curtain position
        setCurtain(!curtainIsOpen);
        printState("MANUAL OVERRIDE - button pressed");
      } else {
        printState("Back to AUTOMATIC mode");
      }
    }
  }

  // ----- 3. Automatic control with hysteresis -----
  if (!manualMode) {
    if (isBright && !curtainIsOpen) {
      setCurtain(true);
      printState("Bright light detected");
    } else if (isDark && curtainIsOpen) {
      setCurtain(false);
      printState("Darkness detected");
    }
  }

  // ----- 4. Show the live light reading once per second while idle -----
  static unsigned long lastPrintMs = 0;
  if (millis() - lastPrintMs >= 1000) {
    lastPrintMs = millis();
    Serial.print("Light: ");
    Serial.print(lightValue);
    Serial.print(" | Mode: ");
    Serial.print(manualMode ? "MANUAL" : "AUTO");
    Serial.print(" | Curtain: ");
    Serial.println(curtainIsOpen ? "OPEN" : "CLOSED");
  }

  delay(20);  // Small delay keeps the loop steady and the analog reading stable
}
