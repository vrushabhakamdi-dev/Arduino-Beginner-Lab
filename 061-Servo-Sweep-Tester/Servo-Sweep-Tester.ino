/*
 * 061 - Servo Sweep Tester
 * Board: Arduino Uno
 *
 * Sweeps an SG90 servo from 0 to 180 degrees and back, one degree at a
 * time, printing the current angle on the Serial Monitor.
 *
 * Wiring:
 *   Servo brown/black wire -> GND
 *   Servo red wire         -> 5V
 *   Servo orange/yellow wire -> digital pin 9 (PWM signal)
 */

#include <Servo.h>   // Servo (built-in) library, included with the Arduino IDE

// ---- Named pin constants ----
const int SERVO_PIN = 9;   // signal wire of the SG90 servo

// ---- Sweep settings ----
const int MIN_ANGLE = 0;    // fully clockwise position
const int MAX_ANGLE = 180;  // fully counter-clockwise position
const int STEP_DELAY = 15;  // milliseconds to wait between each 1 degree step

Servo myServo;  // object that controls the servo

void setup()
{
  Serial.begin(9600);            // start Serial Monitor at 9600 baud
  Serial.println("Servo Sweep Tester");
  Serial.println("Sweeping 0 to 180 degrees...");

  myServo.attach(SERVO_PIN);     // connect the servo to pin 9
  myServo.write(MIN_ANGLE);      // start at 0 degrees
  delay(500);                    // give the servo a moment to reach start
}

void loop()
{
  // ---- Sweep upwards: 0 -> 180, one degree at a time ----
  for (int angle = MIN_ANGLE; angle <= MAX_ANGLE; angle++)
  {
    myServo.write(angle);        // move servo to this angle
    Serial.print("Angle: ");
    Serial.println(angle);       // report the angle to the Serial Monitor
    delay(STEP_DELAY);           // slow the sweep so it is easy to watch
  }

  // ---- Sweep downwards: 180 -> 0, one degree at a time ----
  for (int angle = MAX_ANGLE; angle >= MIN_ANGLE; angle--)
  {
    myServo.write(angle);
    Serial.print("Angle: ");
    Serial.println(angle);
    delay(STEP_DELAY);
  }

  Serial.println("--- Sweep complete, repeating ---");
  delay(500);                    // short pause between full sweeps
}
