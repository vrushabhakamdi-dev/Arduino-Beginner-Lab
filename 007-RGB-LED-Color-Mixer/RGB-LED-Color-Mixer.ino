// 007 - RGB LED Color Mixer
// Three potentiometers independently control the red, green, and blue
// channels of a common-cathode RGB LED, mixing any colour you like.
// The Serial Monitor prints the current 0-255 value of each channel.
// Board: Arduino Uno
// Wiring: Common-cathode RGB LED: longest pin -> GND.
//         Red  -> pin 9  through 220 ohm
//         Green -> pin 10 through 220 ohm
//         Blue  -> pin 11 through 220 ohm
//         Pot 1 (Red)   wiper -> A0, other legs -> 5V and GND
//         Pot 2 (Green) wiper -> A1, other legs -> 5V and GND
//         Pot 3 (Blue)  wiper -> A2, other legs -> 5V and GND

// Output pin constants (9, 10 and 11 are PWM-capable on the Uno)
const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

// Input pin constants, one per potentiometer
const int POT_RED_PIN = A0;
const int POT_GREEN_PIN = A1;
const int POT_BLUE_PIN = A2;

int lastRed = -1;   // Previous values, used to avoid spamming Serial
int lastGreen = -1;
int lastBlue = -1;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // The three potentiometer pins are analogue inputs, which is the default,
  // so no pinMode() call is required for them.
  Serial.begin(9600);
  Serial.println("RGB LED Color Mixer ready. Turn the three knobs.");
}

// Read one potentiometer and return a 0-255 channel value
int readChannel(int potPin) {
  int raw = analogRead(potPin);                 // 0 - 1023
  return map(raw, 0, 1023, 0, 255);             // 0 - 255
}

void loop() {
  // Sample each colour channel from its own potentiometer
  int redValue = readChannel(POT_RED_PIN);
  int greenValue = readChannel(POT_GREEN_PIN);
  int blueValue = readChannel(POT_BLUE_PIN);

  // Drive the RGB LED with the mixed values
  analogWrite(RED_PIN, redValue);
  analogWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, blueValue);

  // Print only when a channel has actually changed, to keep output readable
  if (redValue != lastRed || greenValue != lastGreen || blueValue != lastBlue) {
    Serial.print("RGB = ");
    Serial.print(redValue);
    Serial.print(", ");
    Serial.print(greenValue);
    Serial.print(", ");
    Serial.println(blueValue);

    lastRed = redValue;
    lastGreen = greenValue;
    lastBlue = blueValue;
  }

  delay(50);  // Short pause stabilises the readings and the Serial output
}
