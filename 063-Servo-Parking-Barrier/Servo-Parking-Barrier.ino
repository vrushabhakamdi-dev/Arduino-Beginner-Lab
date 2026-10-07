/*
 * 063 - Servo Parking Barrier
 * Board: Arduino Uno
 *
 * An FC-51 infrared obstacle sensor detects an approaching "car". When an
 * obstacle is seen, the servo on pin 6 lifts the barrier arm from 0 to 90
 * degrees, a warning LED lights up, the arm stays open for 5 seconds, and
 * then it lowers again.
 *
 * Wiring:
 *   FC-51 VCC  -> 5V
 *   FC-51 GND  -> GND
 *   FC-51 OUT  -> digital pin 2
 *   Servo brown/black -> GND
 *   Servo red         -> 5V
 *   Servo orange      -> digital pin 6 (PWM signal)
 *   LED anode (+)     -> digital pin 13 through 220 ohm resistor
 *   LED cathode (-)   -> GND   (pin 13 has a built-in LED on the Uno)
 */

#include <Servo.h>   // Servo (built-in) library

// ---- Named pin constants ----
const int IR_PIN = 2;        // FC-51 obstacle sensor output
const int SERVO_PIN = 6;     // servo that raises the barrier arm
const int WARNING_LED = 13;  // built-in LED, on while the barrier is open

// ---- Barrier positions ----
const int CLOSED_ANGLE = 0;   // arm down, blocking the lane
const int OPEN_ANGLE = 90;    // arm up, lane clear
const int OPEN_WAIT_MS = 5000; // how long the gate stays open

// ---- Sensor logic ----
// Most FC-51 modules output LOW when an obstacle is detected.
const int OBSTACLE_DETECTED = LOW;

Servo barrier;

void setup()
{
  Serial.begin(9600);
  Serial.println("Servo Parking Barrier");

  pinMode(IR_PIN, INPUT);       // sensor drives this pin
  pinMode(WARNING_LED, OUTPUT);

  barrier.attach(SERVO_PIN);
  barrier.write(CLOSED_ANGLE);  // start with the gate shut
  digitalWrite(WARNING_LED, LOW);

  Serial.println("Barrier closed. Waiting for a vehicle...");
}

void loop()
{
  // Read the sensor: LOW means a car is in front of the barrier.
  int sensorValue = digitalRead(IR_PIN);

  if (sensorValue == OBSTACLE_DETECTED)
  {
    Serial.println("Vehicle detected -> lifting barrier");

    barrier.write(OPEN_ANGLE);          // raise the arm
    digitalWrite(WARNING_LED, HIGH);    // warn people the lane is open
    delay(800);                         // time for the arm to swing up

    Serial.println("Barrier open for 5 seconds");
    delay(OPEN_WAIT_MS);                // let the "car" drive through

    Serial.println("Timer finished -> lowering barrier");
    barrier.write(CLOSED_ANGLE);        // drop the arm back down
    digitalWrite(WARNING_LED, LOW);     // warning lamp off
    delay(800);                         // time for the arm to swing down

    Serial.println("Barrier closed. Waiting for a vehicle...");
    delay(500);                         // small debounce pause
  }

  delay(50);   // poll the sensor 20 times per second
}
