/*
 * 070 - Temperature Controlled Fan
 * Board: Arduino Uno
 *
 * A TMP36 sensor measures the room temperature and the Arduino switches a
 * DC fan on or off through an L298N driver:
 *
 *   below 27 degC  -> fan OFF
 *   27 to 32 degC  -> fan LOW speed
 *   above 32 degC  -> fan HIGH speed
 *
 * A red LED wired to pin 6 lights up whenever the temperature is inside
 * the hot zone (27 degC and above). Pin 6 is also L298N input IN1, which
 * only goes HIGH while the fan is running, so the LED and the driver share
 * the pin safely - the Arduino output easily drives both the LED through
 * its resistor and the high-impedance L298N logic input.
 *
 * Wiring (L298N module):
 *   ENA -> digital pin 5 (PWM speed, remove the ENA jumper cap)
 *   IN1 -> digital pin 6 (also feeds the red hot-zone LED)
 *   IN2 -> digital pin 7
 *   OUT1/OUT2 -> DC fan terminals
 *   +12V -> external 6-12V supply (+) if the fan is not a 5V fan
 *   GND  -> supply (-) AND Arduino GND (common ground)
 *
 *   TMP36: flat face to the left, pinout left->right = VCC(5V), Vout(A0), GND
 *   Red LED: pin 6 -> 220 ohm -> LED -> GND
 */

// ---- Named pin constants ----
const int TEMP_PIN = A0;     // TMP36 analogue output
const int ENA_PIN = 5;       // L298N enable A (PWM speed)
const int IN1_PIN = 6;       // L298N input 1 (shared with hot-zone LED)
const int IN2_PIN = 7;       // L298N input 2
const int HOT_LED = 6;       // red LED = hot zone indicator (same pin as IN1)

// ---- Temperature thresholds in degrees Celsius ----
const float LOW_THRESHOLD = 27.0;   // fan starts at this temperature
const float HIGH_THRESHOLD = 32.0;  // fan runs at full speed above this

// ---- Fan speeds (PWM 0-255) ----
const int FAN_OFF = 0;
const int FAN_LOW = 110;
const int FAN_HIGH = 255;

const int SAMPLES = 10;      // readings averaged for a stable temperature

void setup()
{
  Serial.begin(9600);
  Serial.println("Temperature Controlled Fan");
  Serial.println("Temp (C)   Fan State");

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(HOT_LED, OUTPUT);

  // Fixed direction: IN1 HIGH and IN2 LOW makes the fan blow forward.
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN1_PIN, LOW);   // fan off, hot LED off at start
  analogWrite(ENA_PIN, FAN_OFF);
}

// Read the TMP36 and return the temperature in degrees Celsius.
// TMP36: 500 mV at 25 degC and 10 mV per degree C.
float readTemperature()
{
  long total = 0;
  for (int i = 0; i < SAMPLES; i++)
  {
    total += analogRead(TEMP_PIN);
    delay(2);
  }
  float average = total / (float)SAMPLES;

  float volts = average * (5.0 / 1024.0);   // convert counts to volts
  float tempC = (volts - 0.5) * 100.0;      // convert volts to degC
  return tempC;
}

void loop()
{
  float temperature = readTemperature();
  String fanState;

  if (temperature < LOW_THRESHOLD)
  {
    // ---- Too cool: switch the fan off completely ----
    analogWrite(ENA_PIN, FAN_OFF);
    digitalWrite(IN1_PIN, LOW);     // also turns the hot-zone LED off
    digitalWrite(IN2_PIN, LOW);
    fanState = "OFF";
  }
  else if (temperature <= HIGH_THRESHOLD)
  {
    // ---- Warm: run the fan slowly ----
    digitalWrite(IN1_PIN, HIGH);    // forward direction + hot LED on
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, FAN_LOW);
    fanState = "LOW";
  }
  else
  {
    // ---- Hot: run the fan at full speed ----
    digitalWrite(IN1_PIN, HIGH);    // forward direction + hot LED on
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, FAN_HIGH);
    fanState = "HIGH";
  }

  /*
   * HYSTERESIS NOTE:
   * Without hysteresis, a temperature that hovers exactly on a threshold
   * (for example 27.0 degC) would switch the fan on and off many times per
   * second, which is hard on the motor. Hysteresis fixes this by using a
   * different temperature to switch OFF than to switch ON, for example:
   *   switch ON  when temperature >= 27.0
   *   switch OFF when temperature <  26.0   <- 1 degree lower
   * The gap between the two values is the hysteresis band and stops the
   * rapid chatter. The thresholds above are kept at the values specified
   * for this project, so add the band here if your sensor sits right on a
   * limit.
   */

  Serial.print(temperature, 1);
  Serial.print(" C     Fan: ");
  Serial.println(fanState);

  delay(500);   // check the temperature twice a second
}
