// 022 - Parking Distance Assistant
// A car-style parking aid: four LEDs light up as an object gets closer and a
// buzzer beeps faster the nearer the obstacle becomes.
// Board: Arduino Uno
// Wiring:
//   HC-SR04 VCC  -> 5V,  GND -> GND
//   HC-SR04 Trig -> digital pin 9,  HC-SR04 Echo -> digital pin 10
//   LED far      -> pin 4 (with 220 ohm resistor to GND)
//   LED medium   -> pin 5 (with 220 ohm resistor to GND)
//   LED close    -> pin 6 (with 220 ohm resistor to GND)
//   LED very close -> pin 7 (with 220 ohm resistor to GND)
//   Buzzer (+)   -> pin 8,  Buzzer (-) -> GND

// ---------------- Pin constants ----------------
const int TRIG_PIN = 9;    // Ultrasonic trigger output
const int ECHO_PIN = 10;   // Ultrasonic echo input

const int LED_FAR_PIN = 4;        // Lit when the object is beyond 50 cm
const int LED_MEDIUM_PIN = 5;     // Lit when the object is within 35 cm
const int LED_CLOSE_PIN = 6;      // Lit when the object is within 20 cm
const int LED_VERY_CLOSE_PIN = 7; // Lit when the object is within 10 cm

const int BUZZER_PIN = 8;  // Piezo buzzer driven from a digital pin

// ---------------- Threshold constants (centimetres) ----------------
const float THRESHOLD_FAR = 50.0;
const float THRESHOLD_MEDIUM = 35.0;
const float THRESHOLD_CLOSE = 20.0;
const float THRESHOLD_VERY_CLOSE = 10.0;

// ---------------- Timing constants ----------------
const unsigned long SAMPLE_INTERVAL_MS = 60;   // New measurement every 60 ms
const unsigned long BEAT_FAR_MS = 800;         // Slow beep when far away
const unsigned long BEAT_MEDIUM_MS = 500;
const unsigned long BEAT_CLOSE_MS = 250;
const unsigned long BEAT_VERY_CLOSE_MS = 100;  // Fastest beep when very close

// Speed of sound conversion, round trip divided by two
const float MICROSECONDS_TO_CM = 0.0343 / 2.0;

// Tracking variables for non-blocking timing
unsigned long lastSampleTime = 0;
unsigned long lastBeepTime = 0;
bool beepState = false;
unsigned long lastBeepPeriod = 0;  // 0 means "beep off", otherwise ms per beep cycle

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // The four distance LEDs are outputs
  pinMode(LED_FAR_PIN, OUTPUT);
  pinMode(LED_MEDIUM_PIN, OUTPUT);
  pinMode(LED_CLOSE_PIN, OUTPUT);
  pinMode(LED_VERY_CLOSE_PIN, OUTPUT);

  // The buzzer is an output too
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Parking Distance Assistant starting...");
  Serial.println("Distance (cm) and warning level:");
}

// Reads one distance in cm from the HC-SR04, returns -1 when no echo returns
float readDistanceCm() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 30 ms timeout so the sketch never hangs when nothing is in front of the sensor
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1.0;  // No echo: object out of range or sensor miswired
  }
  return duration * MICROSECONDS_TO_CM;
}

// Lights exactly the LEDs that should be on for the current distance
void updateLeds(float distanceCm) {
  // digitalWrite() with a boolean: HIGH when the test is true, LOW otherwise
  digitalWrite(LED_FAR_PIN, distanceCm <= THRESHOLD_FAR);
  digitalWrite(LED_MEDIUM_PIN, distanceCm <= THRESHOLD_MEDIUM);
  digitalWrite(LED_CLOSE_PIN, distanceCm <= THRESHOLD_CLOSE);
  digitalWrite(LED_VERY_CLOSE_PIN, distanceCm <= THRESHOLD_VERY_CLOSE);
}

// Returns how long one beep cycle should be at the current distance
unsigned long beepPeriodFor(float distanceCm) {
  if (distanceCm <= THRESHOLD_VERY_CLOSE) return BEAT_VERY_CLOSE_MS;
  if (distanceCm <= THRESHOLD_CLOSE) return BEAT_CLOSE_MS;
  if (distanceCm <= THRESHOLD_MEDIUM) return BEAT_MEDIUM_MS;
  if (distanceCm <= THRESHOLD_FAR) return BEAT_FAR_MS;
  return 0;  // Nothing detected close enough: stay silent
}

void loop() {
  // Take a fresh measurement on a fixed schedule
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = millis();

    float distanceCm = readDistanceCm();

    if (distanceCm < 0.0) {
      // No object in range: switch everything off and stay quiet
      updateLeds(10000.0);
      Serial.println("No object in range");
      digitalWrite(BUZZER_PIN, LOW);
      beepState = false;
      lastBeepTime = millis();
      return;
    }

    // Show the reading with one decimal so small movements are visible
    Serial.print("Distance: ");
    Serial.print(distanceCm, 1);
    Serial.print(" cm");

    // Pick the warning level name that matches the current threshold
    if (distanceCm <= THRESHOLD_VERY_CLOSE) {
      Serial.println("  LEVEL: STOP");
    } else if (distanceCm <= THRESHOLD_CLOSE) {
      Serial.println("  LEVEL: VERY CLOSE");
    } else if (distanceCm <= THRESHOLD_MEDIUM) {
      Serial.println("  LEVEL: CLOSE");
    } else if (distanceCm <= THRESHOLD_FAR) {
      Serial.println("  LEVEL: FAR");
    } else {
      Serial.println("  LEVEL: CLEAR");
    }

    // Refresh the LED bar and remember the beeping rate for this distance
    updateLeds(distanceCm);
    lastBeepPeriod = beepPeriodFor(distanceCm);
    if (lastBeepPeriod == 0) {
      digitalWrite(BUZZER_PIN, LOW);  // Far enough: turn the buzzer off for good
      beepState = false;
      lastBeepTime = millis();
    }
  }

  // Non-blocking beeper: toggle the buzzer when the current period elapses.
  // The beep gets shorter and faster as the object moves closer.
  if (lastBeepPeriod > 0 && millis() - lastBeepTime >= lastBeepPeriod / 2) {
    lastBeepTime = millis();
    beepState = !beepState;
    digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
  }
}
