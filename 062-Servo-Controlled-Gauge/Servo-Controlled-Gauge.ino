/*
 * 062 - Servo Controlled Gauge
 * Board: Arduino Uno
 *
 * A potentiometer acts like the "sensor" of a dashboard gauge. Turning it
 * moves a servo (the needle) from 0 to 180 degrees, and the Serial Monitor
 * shows the value as a percentage.
 *
 * Wiring:
 *   Potentiometer left leg  -> 5V
 *   Potentiometer wiper     -> analog pin A0
 *   Potentiometer right leg -> GND
 *   Servo brown/black wire  -> GND
 *   Servo red wire          -> 5V
 *   Servo orange wire       -> digital pin 9 (PWM signal)
 *
 * Upgrade note: for a real dashboard you can drive a genuine stepper
 * motor instead of the servo, using a ULN2003 driver board, which gives
 * a full 360 degree needle with no need to stop at the end stops.
 */

#include <Servo.h>   // Servo (built-in) library

// ---- Named pin constants ----
const int POT_PIN = A0;     // potentiometer wiper (gauge input)
const int SERVO_PIN = 9;    // servo signal wire (gauge needle)

// ---- Averaging settings ----
const int SAMPLES = 10;     // readings averaged to calm a shaky needle

Servo gaugeNeedle;

void setup()
{
  Serial.begin(9600);
  Serial.println("Servo Controlled Gauge");
  Serial.println("Turn the potentiometer to move the needle.");

  gaugeNeedle.attach(SERVO_PIN);
  gaugeNeedle.write(0);     // park the needle at the start of the scale
  delay(300);
}

void loop()
{
  // Average a few readings so electrical noise does not make the needle jitter.
  long total = 0;
  for (int i = 0; i < SAMPLES; i++)
  {
    total += analogRead(POT_PIN);   // read 0..1023 from the pot
    delay(2);
  }
  int rawValue = total / SAMPLES;

  // Map the pot range (0..1023) onto the servo range (0..180 degrees).
  int angle = map(rawValue, 0, 1023, 0, 180);
  gaugeNeedle.write(angle);

  // Show the position as a percentage, which is how a gauge is usually labelled.
  int percent = map(rawValue, 0, 1023, 0, 100);

  Serial.print("Gauge: ");
  Serial.print(percent);
  Serial.print("%   Angle: ");
  Serial.print(angle);
  Serial.println(" deg");

  delay(50);   // short pause keeps the Serial Monitor readable
}
