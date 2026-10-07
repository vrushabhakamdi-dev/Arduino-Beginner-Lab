/*
 * 069 - Manual Fan Speed Controller
 * Board: Arduino Uno
 *
 * A potentiometer sets the speed of a small DC fan driven through an
 * L298N motor driver, and a four-LED bar graph shows the selected speed
 * level. The Serial Monitor reports the speed as a percentage.
 *
 * The fan can be any small 5V DC fan (or a 6-12V fan on an external
 * supply). Keep the fan current within the L298N rating.
 *
 * Wiring (L298N module):
 *   ENA -> digital pin 5 (PWM speed, remove the ENA jumper cap)
 *   IN1 -> digital pin 6  (set HIGH for one direction)
 *   IN2 -> digital pin 7  (set LOW)
 *   OUT1/OUT2 -> fan terminals
 *   +12V -> external motor supply (+) if the fan needs more than 5V
 *   GND  -> supply (-) AND Arduino GND (common ground)
 *
 *   Potentiometer: outer leg to 5V, wiper to A0, other outer leg to GND
 *   LED bar: pins 12, 11, 10, 9 each -> 220 ohm -> LED -> GND
 */

// ---- Named pin constants ----
const int FAN_POT_PIN = A0;        // potentiometer wiper
const int ENA_PIN = 5;             // L298N enable A (PWM speed)
const int IN1_PIN = 6;             // L298N input 1 (fixed HIGH)
const int IN2_PIN = 7;             // L298N input 2 (fixed LOW)

const int LED_BAR[4] = {12, 11, 10, 9};  // lowest LED first
const int LED_COUNT = 4;

// ---- Settings ----
const int MIN_SPEED = 0;
const int MAX_SPEED = 255;
const int AVERAGES = 8;            // readings averaged for steady output
const int LEVEL_STEP = 100 / LED_COUNT;  // percent per LED (25%)

void setup()
{
  Serial.begin(9600);
  Serial.println("Manual Fan Speed Controller");

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Fixed direction: the fan always blows the same way.
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, MIN_SPEED);   // fan off at start

  for (int i = 0; i < LED_COUNT; i++)
  {
    pinMode(LED_BAR[i], OUTPUT);
    digitalWrite(LED_BAR[i], LOW);
  }

  Serial.println("Fan stopped. Turn the potentiometer to set speed.");
}

// Light the correct number of LEDs for the chosen percentage.
void showLevel(int percent)
{
  int lit = percent / LEVEL_STEP;   // 0 to 4 LEDs
  for (int i = 0; i < LED_COUNT; i++)
  {
    if (i < lit)
      digitalWrite(LED_BAR[i], HIGH);
    else
      digitalWrite(LED_BAR[i], LOW);
  }
}

void loop()
{
  // Average readings so the fan speed and LEDs do not flicker.
  long total = 0;
  for (int i = 0; i < AVERAGES; i++)
  {
    total += analogRead(FAN_POT_PIN);   // 0..1023
    delay(2);
  }
  int potValue = total / AVERAGES;

  // Map the pot onto the PWM range and drive the fan.
  int pwmValue = map(potValue, 0, 1023, MIN_SPEED, MAX_SPEED);
  analogWrite(ENA_PIN, pwmValue);

  int percent = map(potValue, 0, 1023, 0, 100);
  showLevel(percent);

  Serial.print("FAN SPEED: ");
  Serial.print(percent);
  Serial.println("%");

  delay(250);   // refresh about four times a second
}
