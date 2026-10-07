/*
 * 064 - Servo Automatic Door
 * Board: Arduino Uno
 *
 * A PIR motion sensor detects a person walking up to the door. The servo
 * on pin 6 swings the door from 0 (closed) to 90 degrees (open), a passive
 * buzzer beeps while the door moves, and a status LED shows the door is
 * open. After 5 seconds with no new motion the door closes again.
 *
 * Wiring:
 *   PIR VCC   -> 5V
 *   PIR GND   -> GND
 *   PIR OUT   -> digital pin 2
 *   Servo brown/black -> GND
 *   Servo red         -> 5V
 *   Servo orange      -> digital pin 6 (PWM signal)
 *   Passive buzzer +   -> digital pin 8 (through 100 ohm resistor)
 *   Passive buzzer -   -> GND
 *   LED +              -> digital pin 13 (built-in LED on the Uno)
 */

#include <Servo.h>   // Servo (built-in) library

// ---- Named pin constants ----
const int PIR_PIN = 2;        // PIR motion sensor output
const int SERVO_PIN = 6;      // servo that swings the door
const int BUZZER_PIN = 8;     // passive buzzer, beeps while moving
const int DOOR_LED = 13;      // built-in LED, on while the door is open

// ---- Door positions and timing ----
const int CLOSED_ANGLE = 0;    // door shut
const int OPEN_ANGLE = 90;     // door swung wide open
const int OPEN_HOLD_MS = 5000; // stay open for 5 seconds
const int STEP_DELAY_MS = 8;   // pause after each 1 degree step (smooth travel)
const int BEEP_MS = 60;        // length of one warning beep
const int BEEP_GAP_MS = 40;    // silence after each beep
const int BEEP_EVERY = 10;     // beep once every 10 degrees while moving

Servo doorServo;

void setup()
{
  Serial.begin(9600);
  Serial.println("Servo Automatic Door");
  Serial.println("Door closed. Waiting for motion...");

  pinMode(PIR_PIN, INPUT);
  pinMode(DOOR_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  doorServo.attach(SERVO_PIN);
  doorServo.write(CLOSED_ANGLE);   // start with the door shut
  digitalWrite(DOOR_LED, LOW);
  noTone(BUZZER_PIN);              // make sure the buzzer is silent
}

// Short warning beep used while the door is travelling.
void beep()
{
  tone(BUZZER_PIN, 1000);   // 1 kHz tone on the passive buzzer
  delay(BEEP_MS);
  noTone(BUZZER_PIN);
  delay(BEEP_GAP_MS);
}

// Move the door from its current angle to the target, beeping as it goes.
// The door always alternates between fully open and fully closed, so the
// starting angle is simply the opposite end of the travel.
void moveDoor(int targetAngle)
{
  int startAngle = (targetAngle == OPEN_ANGLE) ? CLOSED_ANGLE : OPEN_ANGLE;
  int stepDirection = (targetAngle > startAngle) ? 1 : -1;

  // Move one degree at a time so the motion looks smooth and controlled.
  for (int a = startAngle; a != targetAngle; a += stepDirection)
  {
    doorServo.write(a);

    // Beep every 10 degrees instead of every degree so the warning sounds
    // natural and the travel only takes about one second.
    if (a % BEEP_EVERY == 0)
    {
      beep();
    }
    else
    {
      delay(STEP_DELAY_MS);
    }
  }

  doorServo.write(targetAngle);   // make sure the door lands exactly on target
  noTone(BUZZER_PIN);
}

void loop()
{
  int motion = digitalRead(PIR_PIN);

  if (motion == HIGH)   // PIR outputs HIGH when movement is sensed
  {
    Serial.println("Motion detected -> opening door");
    digitalWrite(DOOR_LED, HIGH);     // show that the door is open
    moveDoor(OPEN_ANGLE);             // beep while swinging open
    Serial.println("Door open for 5 seconds");

    delay(OPEN_HOLD_MS);              // keep the door open
    Serial.println("No new motion -> closing door");
    moveDoor(CLOSED_ANGLE);           // beep while swinging shut
    digitalWrite(DOOR_LED, LOW);

    Serial.println("Door closed. Waiting for motion...");
    delay(1000);                      // let the PIR settle again
  }

  delay(50);   // poll the sensor 20 times per second
}
