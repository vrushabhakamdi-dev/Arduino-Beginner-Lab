// 006 - Potentiometer LED Dimmer
// Turn the potentiometer knob to smoothly dim and brighten an LED.
// The Serial Monitor shows the brightness as a percentage (0-100%).
// Board: Arduino Uno
// Wiring: Potentiometer outer pins -> 5V and GND, wiper (middle) -> A0.
//         LED anode -> pin 9 through a 220 ohm resistor, cathode -> GND.

// Pin and range constants
const int POT_PIN = A0;     // Potentiometer wiper on analog input A0
const int LED_PIN = 9;      // LED on pin 9, which supports PWM (marked with ~)
const int PWM_MIN = 0;      // Fully off
const int PWM_MAX = 255;    // Fully on (8-bit PWM range)

int lastPercent = -1;       // Used so we only print when the value really changes

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Potentiometer LED Dimmer ready. Turn the knob!");
}

void loop() {
  // Read the pot: analogRead() returns 0 (min) to 1023 (max)
  int potValue = analogRead(POT_PIN);

  // Map the 0-1023 input range onto the 0-255 PWM output range
  int brightness = map(potValue, 0, 1023, PWM_MIN, PWM_MAX);

  // analogWrite() uses PWM to fake an intermediate voltage, which
  // makes the LED appear dimmer or brighter to your eye
  analogWrite(LED_PIN, brightness);

  // Convert the duty cycle to a friendly 0-100% figure
  int percent = map(brightness, PWM_MIN, PWM_MAX, 0, 100);

  // Only send a new line when the percentage has actually changed
  if (percent != lastPercent) {
    Serial.print("Brightness: ");
    Serial.print(percent);
    Serial.println("%");
    lastPercent = percent;
  }

  delay(50);  // Small pause keeps the Serial Monitor readable
}
