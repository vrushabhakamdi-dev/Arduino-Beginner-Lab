// 087 - Automatic Object Sorting Machine
// A small conveyor belt carries objects past an ultrasonic height sensor.
// Tall objects are pushed into bin A by a servo gate, short objects pass
// straight on into bin B, and every decision is logged on the Serial Monitor.
// Board: Arduino Uno
//
// Wiring:
//   L298N conveyor channel (only channel A is used):
//     ENA -> digital pin 5  (PWM conveyor speed, remove the ENA jumper cap)
//     IN1 -> digital pin 6, IN2 -> digital pin 7   (conveyor DC motor)
//     OUT1/OUT2 -> conveyor motor terminals
//     +12V -> separate 6-12V motor battery pack (+)
//     GND  -> battery (-) AND Arduino GND (common ground is essential)
//     Channel B is not used at all: pins 9 and 10 (the usual IN3/ENB pins)
//     belong to the ultrasonic sensor in this project, so leave the L298N
//     IN3, IN4 and ENB terminals unconnected and take the ENB jumper cap off.
//   HC-SR04 TRIG -> digital pin 9, ECHO -> digital pin 10, VCC -> 5V, GND -> GND
//   Sorting gate servo signal -> digital pin 11 (5V and GND too)
//   LED -> digital pin 12 -> 220 ohm -> LED -> GND (tall / short indicator)
//
// SENSOR PLACEMENT: mount the HC-SR04 about 30-40 cm above the belt, looking
// straight down at the sorting station. The empty belt returns roughly that
// distance, and the reading drops when an object passes underneath - so a
// SMALL distance means a TALL object.
//
// GATE: 0 degrees leaves the path open (objects roll on to bin B),
//       90 degrees swings the gate across and diverts them into bin A.
//
// IMPORTANT: power the conveyor and the servo from the external supply, not
// from the Arduino 5V pin, and always join the grounds together.

#include <Servo.h>  // Built-in Arduino library for RC servo motors

// ---------------- Pin constants ----------------
const int ENA_PIN = 5;   // L298N enable A (PWM) - conveyor speed
const int IN1_PIN = 6;   // L298N conveyor input 1
const int IN2_PIN = 7;   // L298N conveyor input 2
const int TRIG_PIN = 9;   // HC-SR04 trigger output
const int ECHO_PIN = 10;  // HC-SR04 echo input
const int GATE_SERVO_PIN = 11;  // sorting gate servo
const int LED_PIN = 12;   // tall / short indicator

// ---------------- Behaviour settings ----------------
const int CONVEYOR_SPEED = 190;   // 0-255, conveyor runs constantly
const int TALL_CM = 10;           // closer than 10 cm from above = TALL object
const int OBJECT_CM = 35;         // closer than 35 cm = something is on the belt
const int GATE_BIN_A_ANGLE = 90;  // gate swung across -> bin A (tall)
const int GATE_BIN_B_ANGLE = 0;   // gate straight      -> bin B (short)
const unsigned long GATE_HOLD_MS = 1200;   // keep the gate swung for 1.2 s
const unsigned long CLEAR_CONFIRM_MS = 250;  // belt must be empty this long
const unsigned long ECHO_TIMEOUT_US = 30000;  // 30 ms ≈ max 5 m, stops pulseIn() hanging

// ---------------- State machine ----------------
enum SortState { WAIT_OBJECT, GATE_ACTIVE, WAIT_CLEAR };
SortState state = WAIT_OBJECT;
unsigned long stateStartMs = 0;
unsigned long lastMeasureMs = 0;
unsigned long clearSinceMs = 0;
int sortCount = 0;         // number of objects sorted so far
int lastDistance = -1;     // last height reading, -1 = belt empty
bool gateAtBinA = false;   // where the gate is currently pointing

Servo gateServo;

// Measure the distance in centimetres, or return -1 when no echo comes back
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1;      // nothing below the sensor
  return (long)(duration / 58);      // ~58 microseconds per cm round trip
}

// Start the conveyor (channel A only, it simply runs all the time)
void conveyorRun() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, CONVEYOR_SPEED);
}

void setup() {
  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  gateServo.attach(GATE_SERVO_PIN);
  gateServo.write(GATE_BIN_B_ANGLE);   // start with the gate open to bin B

  conveyorRun();   // the belt starts moving and never stops

  Serial.begin(9600);
  Serial.println("087 - Automatic Object Sorting Machine ready.");
  Serial.println("Tall object (<= 10 cm) -> BIN A, short object -> BIN B.");
}

void loop() {
  unsigned long now = millis();

  // Measure at most every 80 ms so the echo has time to settle
  if (now - lastMeasureMs >= 80) {
    lastMeasureMs = now;
    lastDistance = readDistanceCm();
  }

  bool objectPresent = (lastDistance > 0 && lastDistance <= OBJECT_CM);
  bool isTall = (lastDistance > 0 && lastDistance <= TALL_CM);

  switch (state) {
    // ----- 1. Watch the belt until an object reaches the station -----
    case WAIT_OBJECT:
      if (objectPresent) {
        sortCount++;

        if (isTall) {
          // Tall: swing the gate across so the object is diverted to bin A
          gateServo.write(GATE_BIN_A_ANGLE);
          gateAtBinA = true;
          digitalWrite(LED_PIN, HIGH);
          Serial.print("Sort #");
          Serial.print(sortCount);
          Serial.print(": TALL object (");
          Serial.print(lastDistance);
          Serial.println(" cm) -> GATE TO BIN A, LED ON");
        } else {
          // Short: leave the gate alone so the object rolls on to bin B
          gateServo.write(GATE_BIN_B_ANGLE);
          gateAtBinA = false;
          digitalWrite(LED_PIN, LOW);
          Serial.print("Sort #");
          Serial.print(sortCount);
          Serial.print(": SHORT object (");
          Serial.print(lastDistance);
          Serial.println(" cm) -> GATE OPEN FOR BIN B, LED OFF");
        }

        state = GATE_ACTIVE;
        stateStartMs = now;
      }
      break;

    // ----- 2. Hold the gate while the object rolls past it -----
    case GATE_ACTIVE:
      if (now - stateStartMs >= GATE_HOLD_MS) {
        gateServo.write(GATE_BIN_B_ANGLE);   // always reset to the neutral path
        gateAtBinA = false;
        digitalWrite(LED_PIN, LOW);
        state = WAIT_CLEAR;
        clearSinceMs = now;
        Serial.println("Gate returned to neutral, waiting for a clear belt.");
      }
      break;

    // ----- 3. Do not count the same object twice -----
    case WAIT_CLEAR:
      if (!objectPresent) {
        if (clearSinceMs == 0) clearSinceMs = now;
        if (now - clearSinceMs >= CLEAR_CONFIRM_MS) {
          state = WAIT_OBJECT;
          clearSinceMs = 0;
          Serial.println("Belt clear - ready for the next object.");
        }
      } else {
        clearSinceMs = now;   // object still there, restart the confirm window
      }
      break;
  }

  // ----- Heartbeat: height reading, gate state and sort count -----
  static unsigned long lastPrintMs = 0;
  if (now - lastPrintMs >= 500) {
    lastPrintMs = now;

    Serial.print("Height sensor: ");
    if (lastDistance < 0) {
      Serial.print("belt empty");
    } else {
      Serial.print(lastDistance);
      Serial.print(" cm");
    }
    Serial.print(" | Gate: ");
    Serial.print(gateAtBinA ? "BIN A" : "BIN B");
    Serial.print(" | Sorted: ");
    Serial.println(sortCount);
  }

  delay(20);
}
