// 030 - Gas Leak Alarm
// Continuously monitors an MQ-2 gas sensor and raises a buzzer and red LED
// alarm when the analog reading crosses a chosen threshold.
// Board: Arduino Uno
// Wiring:
//   MQ-2 module:
//     VCC -> 5V
//     GND -> GND
//     AO  (analog output) -> analog pin A0
//     DO is not used in this project
//   Buzzer (+) -> digital pin 8,  Buzzer (-) -> GND
//   Red LED -> digital pin 6 through a 220 ohm resistor to GND
//
// CRITICAL - sensor burn-in and warm-up:
//   A brand new MQ-2 needs 24 to 48 hours of continuous power before its
//   readings stabilise (burn-in). After every power-up it also needs about
//   1-2 minutes of warm-up before readings are meaningful. The heater inside
//   the sensor gets hot - never touch the mesh while powered. Treat this
//   sketch as an educational prototype: do not use it as a real gas alarm.

// ---------------- Pin constants ----------------
const int GAS_SENSOR_PIN = A0;  // Analog output of the MQ-2 module
const int BUZZER_PIN = 8;       // Piezo buzzer for the audible warning
const int RED_LED_PIN = 6;      // Red LED as the visual warning

// ---------------- Threshold and timing constants ----------------
// The raw value at which the alarm sounds. Fresh air on a new sensor is
// usually around 100-300; adjust after you watch your own baseline in Serial.
const int GAS_THRESHOLD = 500;

const int ADC_MAX = 1023;             // Full-scale 10-bit ADC value
const unsigned long SAMPLE_MS = 250;   // New reading every 250 ms
const unsigned long WARMUP_MS = 60000; // Ignore alarm logic for the first 60 s
const unsigned long BLINK_MS = 150;    // Alarm flash period

unsigned long startTime = 0;
unsigned long lastSampleTime = 0;
unsigned long lastFlashTime = 0;
bool alarmActive = false;
bool ledOn = false;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Both outputs start OFF so nothing sounds during start-up
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Gas Leak Alarm starting...");
  Serial.println("WARM-UP: let the MQ-2 settle for at least 1-2 minutes now.");
  Serial.println("New sensors also need 24-48 hours of burn-in before they settle.");
  Serial.println("analog\tstate");

  startTime = millis();
}

void loop() {
  // ---- Warm-up phase: report values but keep the alarm disabled ----
  bool warmingUp = (millis() - startTime) < WARMUP_MS;

  if (millis() - lastSampleTime >= SAMPLE_MS) {
    lastSampleTime = millis();

    // Read the MQ-2 continuously: higher values mean more reducing gases
    int gasValue = analogRead(GAS_SENSOR_PIN);

    // Decide the alarm state, unless we are still warming up
    if (warmingUp) {
      alarmActive = false;
    } else {
      alarmActive = (gasValue >= GAS_THRESHOLD);
    }

    // Serial output: the raw reading plus a clear state word
    Serial.print(gasValue);
    Serial.print("\t");
    if (warmingUp) {
      Serial.print("WARMING UP (");
      Serial.print((WARMUP_MS - (millis() - startTime)) / 1000);
      Serial.println(" s left)");
    } else if (alarmActive) {
      Serial.println("ALARM - gas level high!");
    } else {
      Serial.println("OK - normal");
    }

    // Turn both warning outputs off whenever the alarm is not active
    if (!alarmActive) {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
      ledOn = false;
    }
  }

  // ---- Alarm phase: flash LED and buzzer together ----
  if (alarmActive && millis() - lastFlashTime >= BLINK_MS) {
    lastFlashTime = millis();
    ledOn = !ledOn;
    digitalWrite(RED_LED_PIN, ledOn ? HIGH : LOW);
    digitalWrite(BUZZER_PIN, ledOn ? HIGH : LOW);
  }
}
