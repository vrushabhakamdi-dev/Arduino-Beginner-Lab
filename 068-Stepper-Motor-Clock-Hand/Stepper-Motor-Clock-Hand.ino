/*
 * 068 - Stepper Motor Clock Hand
 * Board: Arduino Uno
 *
 * A 28BYJ-48 stepper motor on pins 8-11 acts as the minute hand of a
 * clock, advancing 6 degrees every simulated minute. Because a real minute
 * is slow to watch, DEMO_MODE runs the hand 60 times faster so a full hour
 * passes in one minute.
 *
 * The sketch is non-blocking: it counts elapsed milliseconds with millis()
 * and only moves the motor when a simulated minute has finished, so the
 * Serial clock is always up to date.
 *
 * Accuracy note: the 28BYJ-48 has internal gearing and may skip steps if
 * it is asked to move too fast, so a real clock build should keep the
 * speed low, avoid missed steps, and re-sync from a push button now and
 * then. The Arduino's own clock also drifts slightly over long periods.
 *
 * Wiring (ULN2003 driver board):
 *   IN1 -> digital pin 8
 *   IN2 -> digital pin 9
 *   IN3 -> digital pin 10
 *   IN4 -> digital pin 11
 *   VCC -> 5V, GND -> GND
 *   Motor connector -> 28BYJ-48 stepper
 */

#include <Stepper.h>   // Stepper (built-in) library

// ---- Named pin constants ----
const int IN1_PIN = 8;
const int IN2_PIN = 9;
const int IN3_PIN = 10;
const int IN4_PIN = 11;

// ---- Clock settings ----
const int STEPS_PER_REVOLUTION = 2048;  // 28BYJ-48 full revolution
const int MINUTE_ANGLE = 6;             // a minute hand moves 6 degrees

// DEMO_MODE = true  -> 60x speed, one simulated hour every real minute
// DEMO_MODE = false -> real time, one simulated minute every real minute
const bool DEMO_MODE = true;
const long REAL_MINUTE_MS = 60000L;                  // 60 seconds
const long SIM_MINUTE_MS = REAL_MINUTE_MS / 60;      // 1 second in demo

// One minute of clock movement is 6 degrees, which is
// 2048 * 6 / 360 = 34.13 steps. Rounding down to 34 every minute would
// leave the hand about 1.4 degrees behind after each simulated hour, so the
// exact amount is accumulated in pendingSteps, held in units of 1/360 of a
// step, and only whole steps are sent to the motor. After 60 simulated
// minutes the hand has moved exactly 2048 steps, one complete revolution.

Stepper minuteHand(STEPS_PER_REVOLUTION, IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN);

// Simulated clock state, in minutes since midnight.
int simMinutes = 9 * 60;      // start the clock at 09:00
long pendingSteps = 0;        // fractional steps owed to the hand
unsigned long previousMillis = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Stepper Motor Clock Hand");
  Serial.print("Mode: ");
  Serial.println(DEMO_MODE ? "DEMO (60x speed)" : "REAL TIME");

  minuteHand.setSpeed(12);   // revolutions per minute of the shaft

  printClock();
  Serial.println("Clock running...");
}

// Print the simulated time as HH:MM in a leading-zero format.
void printClock()
{
  int hours = (simMinutes / 60) % 24;
  int minutes = simMinutes % 60;

  if (hours < 10) Serial.print('0');
  Serial.print(hours);
  Serial.print(':');
  if (minutes < 10) Serial.print('0');
  Serial.println(minutes);
}

void loop()
{
  unsigned long currentMillis = millis();

  // Move only when a whole simulated minute has elapsed (non-blocking).
  if (currentMillis - previousMillis >= SIM_MINUTE_MS)
  {
    previousMillis += SIM_MINUTE_MS;   // keep accurate timing, no drift
    simMinutes++;                      // advance the simulated clock

    // Advance the physical hand by 6 degrees for each simulated minute,
    // carrying the fraction forward so the hand never falls behind.
    pendingSteps += (long)STEPS_PER_REVOLUTION * MINUTE_ANGLE;
    long moveSteps = pendingSteps / 360;
    pendingSteps -= moveSteps * 360;
    minuteHand.step(moveSteps);

    Serial.print("Minute hand advanced -> ");
    printClock();
  }

  // Nothing else to do here; millis() keeps the timing in the background.
}
