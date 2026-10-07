// 078 - Water Level Alarm
// Measure how full a container is with a water level sensor module, show the
// level on a four LED bar and sound a buzzer when the tank goes above 80%.
// Board: Arduino Uno
// Wiring:
//   Water level sensor module: VCC -> 5V, GND -> GND, SIG -> pin A0
//   LED bar: 25% -> pin 4, 50% -> pin 5, 75% -> pin 6, 95% -> pin 7
//     (each LED through a 220 ohm resistor to GND)
//   Buzzer -> pin 8
//
// EDUCATION ONLY: this project measures and reports the level for learning.
// It contains no pump and no valve - it must not be used for irrigation or
// for switching real mains equipment.

// ---------------- Pin constants ----------------
const int LEVEL_PIN = A0;             // Analog output of the water level sensor
const int LED_PINS[4] = {4, 5, 6, 7}; // LED bar, lowest threshold first
const int BUZZER_PIN = 8;             // Buzzer sounds when the tank is too full

// ---------------- Calibration ----------------
// The bare-trace sensor rarely reaches the full 1023 counts. Dry it, dunk it to
// the maximum depth, read the Serial values, and put them here for a 0-100% scale.
const int RAW_DRY = 0;      // Reading with the sensor out of the water
const int RAW_WET = 1023;   // Reading at the maximum intended depth

const int LED_THRESHOLDS[4] = {25, 50, 75, 95};  // Percent for each LED
const int ALARM_PERCENT = 80;   // Above this level the fill alarm sounds
const unsigned long ALARM_BLINK_MS = 300;  // Alarm blinks about 3 times a second

// ---------------- State variables ----------------
int currentPercent = 0;
bool alarmOn = false;
unsigned long lastBlinkMs = 0;
bool blinkOn = false;

// Convert the raw analog reading into a 0-100% level
int readPercent() {
  int raw = analogRead(LEVEL_PIN);
  if (raw < RAW_DRY) raw = RAW_DRY;
  if (raw > RAW_WET) raw = RAW_WET;
  return map(raw, RAW_DRY, RAW_WET, 0, 100);
}

// Text description of the current level for the Serial Monitor
const char *levelState() {
  if (alarmOn) return "FILL ALARM - tank nearly full!";
  if (currentPercent >= 50) return "half full";
  if (currentPercent >= 10) return "low level";
  return "empty";
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  Serial.begin(9600);
  Serial.println("Water Level Alarm ready (measurement demo - no pump).");
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Read the level -----
  int newPercent = readPercent();
  if (newPercent != currentPercent) {
    currentPercent = newPercent;
  }

  // ----- 2. Update the LED bar -----
  for (int i = 0; i < 4; i++) {
    bool on = (currentPercent >= LED_THRESHOLDS[i]);
    digitalWrite(LED_PINS[i], on ? HIGH : LOW);
  }

  // ----- 3. Fill alarm above 80% -----
  bool shouldAlarm = (currentPercent > ALARM_PERCENT);
  if (shouldAlarm != alarmOn) {
    alarmOn = shouldAlarm;
    if (alarmOn) {
      Serial.println("ALARM: level passed 80%!");
    } else {
      noTone(BUZZER_PIN);
      Serial.println("Alarm cleared - level below 80%.");
    }
  }

  if (alarmOn) {
    // Flash the buzzer instead of one long tone so it is easy to hear
    if (now - lastBlinkMs >= ALARM_BLINK_MS) {
      lastBlinkMs = now;
      blinkOn = !blinkOn;
      if (blinkOn) tone(BUZZER_PIN, 2500);
      else noTone(BUZZER_PIN);
    }
  }

  // ----- 4. Serial report once per second -----
  static unsigned long lastPrintMs = 0;
  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.print("Water level: ");
    Serial.print(currentPercent);
    Serial.print(" % | State: ");
    Serial.println(levelState());
  }

  delay(20);
}
