/*
 * 065 - DC Motor Speed Controller
 * Board: Arduino Uno
 *
 * A potentiometer on A0 sets the speed of a DC motor driven through an
 * L298N motor driver. The Serial Monitor reports the chosen speed as a
 * percentage from 0% to 100%.
 *
 * Wiring (L298N module):
 *   ENA -> digital pin 5 (PWM speed control, remove the ENA jumper cap)
 *   IN1 -> digital pin 6  (set HIGH)
 *   IN2 -> digital pin 7  (set LOW)
 *   OUT1/OUT2 -> DC motor terminals
 *   +12V screw terminal -> external 6-12V motor supply (+)
 *   GND screw terminal  -> external supply (-) AND Arduino GND (common ground)
 *   5V screw terminal   -> leave unconnected if the module regulator powers it
 *
 *   Potentiometer: one outer leg to 5V, wiper to A0, other outer leg to GND
 *
 * IMPORTANT: power the motor from a SEPARATE 6-12V supply, never from the
 * Arduino's 5V pin. The motor supply can draw far more current than the
 * Arduino can give. Always tie the supply GND to Arduino GND.
 */

#include <Arduino.h>

// ---- Named pin constants ----
const int SPEED_POT_PIN = A0;   // potentiometer wiper
const int ENA_PIN = 5;          // L298N enable A (PWM speed)
const int IN1_PIN = 6;          // L298N input 1 (direction)
const int IN2_PIN = 7;          // L298N input 2 (direction)

// ---- Settings ----
const int MIN_SPEED = 0;     // motor fully stopped
const int MAX_SPEED = 255;   // full PWM duty cycle
const int AVERAGES = 8;      // readings averaged for a steady speed

void setup()
{
  Serial.begin(9600);
  Serial.println("DC Motor Speed Controller");

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Fixed direction: IN1 HIGH and IN2 LOW makes the motor spin one way.
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);

  analogWrite(ENA_PIN, MIN_SPEED);   // start with the motor stopped
  Serial.println("Motor stopped. Turn the potentiometer to set speed.");
}

void loop()
{
  // Average several analogue readings so the speed does not flicker.
  long total = 0;
  for (int i = 0; i < AVERAGES; i++)
  {
    total += analogRead(SPEED_POT_PIN);   // 0..1023 from the pot
    delay(2);
  }
  int potValue = total / AVERAGES;

  // Map the pot range onto the PWM range and apply it to the driver.
  int pwmValue = map(potValue, 0, 1023, MIN_SPEED, MAX_SPEED);
  analogWrite(ENA_PIN, pwmValue);

  // Report the duty cycle as a friendly percentage.
  int percent = map(potValue, 0, 1023, 0, 100);

  Serial.print("SPEED: ");
  Serial.print(percent);
  Serial.println("%");

  delay(250);   // refresh about four times a second
}
