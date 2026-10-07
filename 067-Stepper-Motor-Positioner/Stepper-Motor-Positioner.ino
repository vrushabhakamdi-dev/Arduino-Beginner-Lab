/*
 * 067 - Stepper Motor Positioner
 * Board: Arduino Uno
 *
 * A 28BYJ-48 stepper motor on a ULN2003 driver board acts as a rotary
 * positioner. Two push buttons turn it +90 degrees or -90 degrees, and the
 * Serial Monitor keeps a running total of the current position in degrees.
 *
 * Wiring (ULN2003 driver board):
 *   IN1 -> digital pin 8
 *   IN2 -> digital pin 9
 *   IN3 -> digital pin 10
 *   IN4 -> digital pin 11
 *   VCC -> 5V
 *   GND -> GND
 *   Motor connector -> 28BYJ-48 stepper (plug straight in)
 *
 *   Buttons (INPUT_PULLUP, no external resistors needed):
 *     +90 degrees button -> digital pin 2, other side to GND
 *     -90 degrees button -> digital pin 3, other side to GND
 */

#include <Stepper.h>   // Stepper (built-in) library

// ---- Named pin constants ----
const int IN1_PIN = 8;
const int IN2_PIN = 9;
const int IN3_PIN = 10;
const int IN4_PIN = 11;

const int CW_BTN = 2;    // button that adds +90 degrees
const int CCW_BTN = 3;   // button that subtracts -90 degrees

// ---- Motor settings ----
// The 28BYJ-48 in half-step mode needs 4096 steps for one full turn;
// in full-step mode it needs 2048. This sketch uses the classic 2048.
const int STEPS_PER_REVOLUTION = 2048;
const int STEP_ANGLE = 90;          // degrees moved per button press
// Multiply first, then divide, so 90 degrees gives exactly 512 steps
// instead of being rounded down to an inaccurate value.
const int STEPS_PER_MOVE = (STEPS_PER_REVOLUTION * STEP_ANGLE) / 360;
const unsigned long DEBOUNCE_MS = 250;
const int BUTTON_PRESSED = LOW;

// Create the stepper object using pins IN1..IN4.
Stepper myStepper(STEPS_PER_REVOLUTION, IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN);

long positionDegrees = 0;        // cumulative position, can be negative
unsigned long lastPressTime = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("Stepper Motor Positioner");

  pinMode(CW_BTN, INPUT_PULLUP);
  pinMode(CCW_BTN, INPUT_PULLUP);

  // Slower speed = more torque and fewer missed steps on the 28BYJ-48.
  myStepper.setSpeed(10);   // 10 revolutions per minute

  Serial.println("Position: 0 degrees. Press a button to move.");
}

// True exactly once per fresh button press.
bool buttonPressed(int pin)
{
  if (digitalRead(pin) == BUTTON_PRESSED &&
      (millis() - lastPressTime) >= DEBOUNCE_MS)
  {
    lastPressTime = millis();
    return true;
  }
  return false;
}

void loop()
{
  if (buttonPressed(CW_BTN))
  {
    // Positive step count turns the shaft clockwise.
    myStepper.step(STEPS_PER_MOVE);
    positionDegrees += STEP_ANGLE;

    Serial.print("Clockwise move -> Position: ");
    Serial.print(positionDegrees);
    Serial.println(" degrees");
  }
  else if (buttonPressed(CCW_BTN))
  {
    // Negative step count turns the shaft counter-clockwise.
    myStepper.step(-STEPS_PER_MOVE);
    positionDegrees -= STEP_ANGLE;

    Serial.print("Counter-clockwise move -> Position: ");
    Serial.print(positionDegrees);
    Serial.println(" degrees");
  }

  delay(20);   // keep the sketch responsive while polling
}
