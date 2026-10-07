// 029 - Flame Detection Alarm
// Detects the infrared signature of a flame with a KY-026 flame sensor and
// sounds a buzzer plus an LED alarm while the flame is present.
// Board: Arduino Uno
// Wiring:
//   KY-026 module:
//     + (VCC) -> 5V
//     - (GND) -> GND
//     S (digital output) -> digital pin 2
//     (the analog output 'A' is not used in this sketch)
//   Buzzer (+) -> digital pin 8,  Buzzer (-) -> GND
//   Alarm LED -> digital pin 13 (built-in LED) through a 220 ohm resistor
//
// IMPORTANT - check your module's polarity first:
//   KY-026 boards vary: some pull the S pin LOW when a flame is seen
//   (active-LOW), others pull it HIGH (active-HIGH). Upload this sketch and
//   watch the Serial Monitor while you bring a lighter flame or bright IR
//   source near the sensor. Note which level means "flame", then set the
//   constant FLAME_LEVEL below to LOW or HIGH to match your board.

// ---------------- Pin constants ----------------
const int FLAME_SENSOR_PIN = 2;  // Digital output of the KY-026
const int BUZZER_PIN = 8;        // Piezo buzzer for the audible alarm
const int ALARM_LED_PIN = 13;    // Built-in LED used as the alarm lamp

// Level the S pin shows while a flame is detected - TEST AND SET THIS.
// Leave it as LOW for the usual active-low KY-026, change to HIGH if needed.
const int FLAME_LEVEL = LOW;

// Alarm flashing timing
const unsigned long BLINK_MS = 200;       // Fast flash while alarm is active
const unsigned long SILENT_CHECK_MS = 500; // Status print when nothing detected

unsigned long lastFlashTime = 0;
bool alarmActive = false;
bool ledOn = false;

void setup() {
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);

  // Make sure both alarm outputs start in the safe OFF state
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(ALARM_LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Flame Detection Alarm starting...");
  Serial.println("Bring a flame near the sensor and check which level changes.");
}

void loop() {
  // Read the sensor's digital output
  int sensorValue = digitalRead(FLAME_SENSOR_PIN);

  // Compare against the level that means "flame detected" on this board
  if (sensorValue == FLAME_LEVEL) {
    // ---- Flame detected: run the alarm ----
    if (!alarmActive) {
      // Print the alert only once, at the moment the flame first appears
      Serial.println("FLAME DETECTED - ALARM ON");
      alarmActive = true;
      lastFlashTime = millis();
      ledOn = true;
    }

    // Flash the LED quickly instead of using delay(), so the loop stays live
    if (millis() - lastFlashTime >= BLINK_MS) {
      lastFlashTime = millis();
      ledOn = !ledOn;
      digitalWrite(ALARM_LED_PIN, ledOn ? HIGH : LOW);
      digitalWrite(BUZZER_PIN, ledOn ? HIGH : LOW);
    }
  } else {
    // ---- No flame: make sure everything is switched off ----
    if (alarmActive) {
      Serial.println("Flame cleared - alarm off");
      alarmActive = false;
    }
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(ALARM_LED_PIN, LOW);
    ledOn = false;

    // Regular heartbeat so you can see the sensor is being polled
    if (millis() - lastFlashTime >= SILENT_CHECK_MS) {
      lastFlashTime = millis();
      Serial.println("Status: no flame. Sensor value = " + String(sensorValue));
    }
  }
}
