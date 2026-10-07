// 076 - Automatic Trash Can
// Wave your hand in front of an IR obstacle sensor and the servo lifts the
// lid of a model trash can for 3 seconds, while an LED shows it is open.
// Board: Arduino Uno
// Wiring:
//   FC-51 IR obstacle sensor: OUT -> pin 2, VCC -> 5V, GND -> GND
//   Servo signal -> pin 6 (power the servo from 5V, common GND)
//   LED (open indicator) -> pin 13 through 220 ohm resistor (or the built-in LED)
//
// Libraries: Servo (built-in with the Arduino IDE)

#include <Servo.h>  // Built-in Arduino library for RC servo motors

// ---------------- Pin constants ----------------
const int SENSOR_PIN = 2;  // FC-51 obstacle sensor output
const int SERVO_PIN = 6;   // Lid servo
const int LED_PIN = 13;    // "Lid open" indicator LED

// ---------------- Behaviour constants ----------------
// Most FC-51 modules pull the OUT pin LOW when something is in front of them,
// but some boards are inverted. Change this one constant if your module
// reports the opposite way.
const bool OBSTACLE_ACTIVE_LOW = true;

const unsigned long LID_OPEN_MS = 3000;   // Lid stays open for 3 seconds
const unsigned long SENSOR_CONFIRM_MS = 80;  // Hand must be seen this long (noise filter)

// ---------------- Servo positions ----------------
const int LID_CLOSED_ANGLE = 0;
const int LID_OPEN_ANGLE = 90;

// ---------------- State variables ----------------
Servo lidServo;
bool lidIsOpen = false;
unsigned long obstacleSinceMs = 0;   // When the hand was first seen (0 = not seen)
unsigned long lidOpenedAtMs = 0;     // When the lid was opened
int openCount = 0;                   // How many times the lid has opened
bool sensorStable = false;           // Debounced sensor state

// Convert a raw sensor reading into "is something in front of the sensor?"
bool obstacleDetected() {
  int raw = digitalRead(SENSOR_PIN);
  return OBSTACLE_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

void openLid() {
  lidServo.write(LID_OPEN_ANGLE);
  lidIsOpen = true;
  digitalWrite(LED_PIN, HIGH);  // Indicator LED on
  openCount++;
  lidOpenedAtMs = millis();

  Serial.print("Lid OPENED (count ");
  Serial.print(openCount);
  Serial.println(")");
}

void closeLid() {
  lidServo.write(LID_CLOSED_ANGLE);
  lidIsOpen = false;
  digitalWrite(LED_PIN, LOW);   // Indicator LED off
  Serial.println("Lid CLOSED");
}

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  lidServo.attach(SERVO_PIN);
  lidServo.write(LID_CLOSED_ANGLE);  // Start with the lid closed
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Automatic Trash Can ready - wave your hand over the sensor.");
}

void loop() {
  bool handSeen = obstacleDetected();
  unsigned long now = millis();

  // ----- 1. Confirm the hand is really there (80 ms noise filter) -----
  if (handSeen && !sensorStable) {
    // Wait until the hand has been visible continuously for SENSOR_CONFIRM_MS
    if (obstacleSinceMs == 0) {
      obstacleSinceMs = now;
    } else if (now - obstacleSinceMs >= SENSOR_CONFIRM_MS) {
      sensorStable = true;
    }
  } else if (!handSeen) {
    obstacleSinceMs = 0;   // Hand removed before confirmation - start over
    sensorStable = false;
  }

  // ----- 2. Open the lid while the hand is present -----
  if (sensorStable && !lidIsOpen) {
    openLid();
  }

  // ----- 3. Close the lid LID_OPEN_MS after the hand disappears -----
  if (lidIsOpen && !sensorStable) {
    if (now - lidOpenedAtMs >= LID_OPEN_MS) {
      closeLid();
    }
  }

  // ----- 4. Keep-alive: if the hand stays there, hold the lid open -----
  if (lidIsOpen && sensorStable) {
    lidOpenedAtMs = now;  // Reset the 3 second timer while the hand is present
  }

  // ----- 5. Status line every second -----
  static unsigned long lastPrintMs = 0;
  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.print("Sensor: ");
    Serial.print(handSeen ? "HAND" : "clear");
    Serial.print(" | Lid: ");
    Serial.print(lidIsOpen ? "OPEN" : "CLOSED");
    Serial.print(" | Opens so far: ");
    Serial.println(openCount);
  }

  delay(20);  // Small delay keeps the loop steady
}
