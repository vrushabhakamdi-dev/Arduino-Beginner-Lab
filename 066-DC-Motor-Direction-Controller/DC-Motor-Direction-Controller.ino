/*
 * 066 - DC Motor Direction Controller
 * Board: Arduino Uno
 *
 * Three push buttons choose the direction of a DC motor driven by an
 * L298N module: FORWARD, REVERSE or STOP. Two LEDs show which direction
 * is active, and every button press is logged on the Serial Monitor.
 *
 * Wiring (L298N module):
 *   ENA -> digital pin 5 (PWM speed, remove the ENA jumper cap)
 *   IN1 -> digital pin 6
 *   IN2 -> digital pin 7
 *   OUT1/OUT2 -> DC motor terminals
 *   +12V -> external 6-12V supply (+)
 *   GND  -> external supply (-) AND Arduino GND (common ground)
 *
 *   Buttons (INPUT_PULLUP, so no external resistors needed):
 *     FORWARD button -> digital pin 2, other side to GND
 *     REVERSE button -> digital pin 3, other side to GND
 *     STOP    button -> digital pin 4, other side to GND
 *
 *   LEDs:
 *     Forward LED -> pin 9  -> 220 ohm -> LED -> GND
 *     Reverse LED -> pin 10 -> 220 ohm -> LED -> GND
 *
 * IMPORTANT: use a separate 6-12V supply for the motor, never the Arduino
 * 5V pin, and connect the supply GND to the Arduino GND.
 */

// ---- Named pin constants ----
const int ENA_PIN = 5;        // L298N enable A (PWM speed)
const int IN1_PIN = 6;        // L298N input 1
const int IN2_PIN = 7;        // L298N input 2

const int FORWARD_BTN = 2;    // button to GND, uses internal pull-up
const int REVERSE_BTN = 3;    // button to GND, uses internal pull-up
const int STOP_BTN = 4;       // button to GND, uses internal pull-up

const int FORWARD_LED = 9;    // lights up when running forward
const int REVERSE_LED = 10;   // lights up when running in reverse

// ---- Settings ----
const int MOTOR_SPEED = 200;  // PWM 0-255 used while running
const int MOTOR_STOP = 0;     // PWM value that stops the motor
const unsigned long DEBOUNCE_MS = 200;  // ignore rapid double presses
const int BUTTON_PRESSED = LOW;         // INPUT_PULLUP reads LOW when pressed

// Track the last time a button was accepted, for debouncing.
unsigned long lastPressTime = 0;

void setup()
{
  Serial.begin(9600);
  Serial.println("DC Motor Direction Controller");

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Internal pull-ups mean the buttons read HIGH until they are pressed.
  pinMode(FORWARD_BTN, INPUT_PULLUP);
  pinMode(REVERSE_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT_PULLUP);

  pinMode(FORWARD_LED, OUTPUT);
  pinMode(REVERSE_LED, OUTPUT);

  stopMotor();
  Serial.println("Ready. Press FORWARD, REVERSE or STOP.");
}

// Stop the motor and turn both direction LEDs off.
void stopMotor()
{
  analogWrite(ENA_PIN, MOTOR_STOP);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(FORWARD_LED, LOW);
  digitalWrite(REVERSE_LED, LOW);
}

// Run the motor in one direction at the chosen speed.
void runForward()
{
  digitalWrite(IN1_PIN, HIGH);   // IN1 HIGH + IN2 LOW = forward
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, MOTOR_SPEED);
  digitalWrite(FORWARD_LED, HIGH);
  digitalWrite(REVERSE_LED, LOW);
}

// Run the motor the opposite way at the chosen speed.
void runReverse()
{
  digitalWrite(IN1_PIN, LOW);    // IN1 LOW + IN2 HIGH = reverse
  digitalWrite(IN2_PIN, HIGH);
  analogWrite(ENA_PIN, MOTOR_SPEED);
  digitalWrite(FORWARD_LED, LOW);
  digitalWrite(REVERSE_LED, HIGH);
}

// Return true exactly once per fresh button press (simple debounce).
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
  if (buttonPressed(FORWARD_BTN))
  {
    runForward();
    Serial.println("Command: FORWARD");
  }
  else if (buttonPressed(REVERSE_BTN))
  {
    runReverse();
    Serial.println("Command: REVERSE");
  }
  else if (buttonPressed(STOP_BTN))
  {
    stopMotor();
    Serial.println("Command: STOP");
  }

  delay(20);   // short polling delay keeps the sketch responsive
}
