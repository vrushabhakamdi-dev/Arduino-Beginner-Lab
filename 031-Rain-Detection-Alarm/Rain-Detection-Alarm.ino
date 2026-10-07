// 031 - Rain Detection Alarm
// Sounds an alarm as soon as rain drops hit the rain drop sensor plate.
// Board: Arduino Uno
// Wiring:
//   Rain drop sensor module  ->  VCC = 5V, GND = GND, DO = pin 2, AO = A0
//   Buzzer (+)               ->  pin 8 through a 220 ohm resistor, buzzer (-) = GND
//   LED anode (+)            ->  pin 4 through a 220 ohm resistor, LED cathode (-) = GND
//
// NOTE: this project is an ALARM only. It never controls a pump or waters anything.

// ---- Pin constants (named constants instead of magic numbers) ----
const int RAIN_DO_PIN = 2;   // Digital output of the rain sensor (HIGH = dry, LOW = rain)
const int RAIN_AO_PIN = A0;  // Analog output of the rain sensor (0 = very wet, 1023 = dry)
const int BUZZER_PIN = 8;    // Piezo buzzer that beeps when it rains
const int LED_PIN = 4;       // Indicator LED that lights up when it rains

// ---- Timing constants ----
const unsigned long SAMPLE_MS = 250;   // How often we read the sensor (ms)
const int DRY_THRESHOLD = 500;         // Analog level below this counts as "raining"

unsigned long lastSample = 0;          // Timestamp of the last sensor read
bool lastRaining = false;              // Previous state, used so we only print changes

void setup() {
  // Digital output of the sensor is an input for the Arduino
  pinMode(RAIN_DO_PIN, INPUT);

  // Buzzer and LED are outputs we drive when rain is detected
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // A0 is an analog input by default, but we say it anyway for clarity
  pinMode(RAIN_AO_PIN, INPUT);

  // Start serial communication so we can watch the readings
  Serial.begin(9600);
  Serial.println("Rain Detection Alarm starting...");

  // Read the sensor once right away so the first line shows the real status
  int initialAnalog = analogRead(RAIN_AO_PIN);
  int initialDigital = digitalRead(RAIN_DO_PIN);
  lastRaining = (initialDigital == LOW) || (initialAnalog <= DRY_THRESHOLD);
  Serial.print("Initial status: ");
  Serial.print(lastRaining ? "RAINING" : "DRY");
  Serial.print("  analog=");
  Serial.println(initialAnalog);
}

void loop() {
  // Only sample the sensor every SAMPLE_MS milliseconds so the Serial Monitor
  // stays readable and the program does not spam output.
  if (millis() - lastSample < SAMPLE_MS) {
    return;  // Not time yet, go back to the top of loop()
  }
  lastSample = millis();

  // ---- Read both sensor outputs ----
  int analogLevel = analogRead(RAIN_AO_PIN);   // 0..1023 raw moisture level
  int digitalState = digitalRead(RAIN_DO_PIN); // Module's own on-board threshold output

  // Water conducts across the plate, so the analog level falls as it gets wetter.
  // We accept EITHER output as proof of rain:
  //   - the digital pin flips LOW as soon as the module's own threshold is crossed
  //   - the analog level drops below our DRY_THRESHOLD number
  bool rainOnDigital = (digitalState == LOW);
  bool rainOnAnalog  = (analogLevel <= DRY_THRESHOLD);
  bool raining = rainOnDigital || rainOnAnalog;

  // ---- Drive the alarm outputs ----
  if (raining) {
    digitalWrite(LED_PIN, HIGH);    // Turn the warning LED on
    tone(BUZZER_PIN, 1000);         // Emit a 1 kHz beep from the buzzer
  } else {
    digitalWrite(LED_PIN, LOW);     // Turn the warning LED off
    noTone(BUZZER_PIN);             // Stop the beeping
  }

  // ---- Serial reporting: print the full status only when the state changes,
  //      and always keep the raw analog level visible for tuning. ----
  if (raining != lastRaining) {
    if (raining) {
      Serial.print("[");
      Serial.print(millis() / 1000);
      Serial.print(" s] RAINING  analog=");
      Serial.println(analogLevel);
    } else {
      Serial.print("[");
      Serial.print(millis() / 1000);
      Serial.print(" s] DRY  analog=");
      Serial.println(analogLevel);
    }
    lastRaining = raining;  // Remember the new state
  } else {
    // Continuous line so you can watch the level change drop by drop
    Serial.print("level=");
    Serial.println(analogLevel);
  }
}
