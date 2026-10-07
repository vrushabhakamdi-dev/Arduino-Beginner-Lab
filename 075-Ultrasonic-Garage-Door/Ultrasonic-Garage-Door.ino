// 075 - Ultrasonic Garage Door
// An HC-SR04 ultrasonic sensor detects an approaching car and a servo raises
// a model garage door. The door stays open while the car is present and for
// 3 seconds afterwards, and a four LED bar shows how close the car is.
// Board: Arduino Uno
// Wiring:
//   HC-SR04 TRIG -> pin 9, HC-SR04 ECHO -> pin 10, VCC -> 5V, GND -> GND
//   Servo signal -> pin 6 (power the servo from 5V, common GND)
//   LED bar: 100 cm -> pin 4, 60 cm -> pin 5, 30 cm -> pin 7, 10 cm -> pin 8
//     (each LED through a 220 ohm resistor to GND)
//
// PIN NOTE: the original idea placed the whole LED bar on pins 4-7, but the
// servo already uses pin 6. The 10 cm LED was therefore moved to pin 8 so the
// servo signal is not shared with an LED.
//
// Libraries: Servo (built-in with the Arduino IDE)

#include <Servo.h>  // Built-in Arduino library for RC servo motors

// ---------------- Pin constants ----------------
const int TRIG_PIN = 9;              // HC-SR04 trigger output
const int ECHO_PIN = 10;             // HC-SR04 echo input
const int SERVO_PIN = 6;             // Garage door servo
const int LED_PINS[4] = {4, 5, 7, 8};  // Distance bar LEDs (closest distance last)

// ---------------- Distance thresholds ----------------
const int LED_THRESHOLDS[4] = {100, 60, 30, 10};  // Light up when a car is this close or closer
const int CAR_PRESENT_CM = 100;   // A car closer than this counts as "arriving"
const unsigned long KEEP_OPEN_MS = 3000;  // Door stays open 3 s after the car leaves
const unsigned long ECHO_TIMEOUT_US = 30000;  // 30 ms ≈ max 5 m, stops pulseIn() hanging

// ---------------- Door positions ----------------
const int DOOR_CLOSED_ANGLE = 0;  // Door fully closed
const int DOOR_OPEN_ANGLE = 90;   // Door fully raised

// ---------------- State variables ----------------
Servo doorServo;
bool doorIsOpen = false;
bool carSeenBefore = false;      // True after the first car has been detected
unsigned long lastCarSeenMs = 0;  // Last time a car was within CAR_PRESENT_CM

// Measure the distance in centimetres, or return -1 when nothing is in range
long readDistanceCm() {
  // Send a clean 10 microsecond trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn() returns the length of the echo pulse in microseconds
  // The timeout keeps the sketch responsive when no object reflects the sound
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1;      // No echo = nothing in range
  return (long)(duration / 58);      // Sound travels ~58 microseconds per cm round trip
}

// Move the door to the requested position (only writes when the state changes)
void setDoor(bool open) {
  if (open == doorIsOpen) return;
  doorIsOpen = open;
  doorServo.write(open ? DOOR_OPEN_ANGLE : DOOR_CLOSED_ANGLE);
  Serial.println(open ? "Garage door: OPENING" : "Garage door: CLOSING");
}

// Update the four LED distance bar
void updateLedBar(long distanceCm) {
  for (int i = 0; i < 4; i++) {
    bool on = (distanceCm > 0 && distanceCm <= LED_THRESHOLDS[i]);
    digitalWrite(LED_PINS[i], on ? HIGH : LOW);
  }
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  doorServo.attach(SERVO_PIN);
  doorServo.write(DOOR_CLOSED_ANGLE);  // Start with the door closed

  Serial.begin(9600);
  Serial.println("Ultrasonic Garage Door ready.");
  Serial.println("Drive a car (or hold an object) within 100 cm of the sensor.");
}

void loop() {
  long distanceCm = readDistanceCm();

  // ----- 1. Remember the last time a car was detected -----
  if (distanceCm > 0 && distanceCm <= CAR_PRESENT_CM) {
    lastCarSeenMs = millis();
    carSeenBefore = true;
  }

  // ----- 2. Open while the car is there, and for 3 seconds after it leaves -----
  bool carRecentlyPresent = carSeenBefore &&
                            (millis() - lastCarSeenMs) <= KEEP_OPEN_MS;
  setDoor(carRecentlyPresent);

  // ----- 3. Distance bar -----
  updateLedBar(distanceCm);

  // ----- 4. Serial feedback once every 500 ms -----
  static unsigned long lastPrintMs = 0;
  if (millis() - lastPrintMs >= 500) {
    lastPrintMs = millis();

    Serial.print("Distance: ");
    if (distanceCm < 0) {
      Serial.print("out of range");
    } else {
      Serial.print(distanceCm);
      Serial.print(" cm");
    }
    Serial.print(" | Door: ");
    Serial.println(doorIsOpen ? "OPEN" : "CLOSED");
  }

  delay(50);  // Short pause so the sensor is not retriggered too fast
}
