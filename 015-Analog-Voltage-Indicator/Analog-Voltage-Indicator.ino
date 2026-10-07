// 015 - Analog Voltage Indicator
// Board: Arduino Uno
// Wiring:
//   Potentiometer (used as an adjustable test voltage):
//     one outer pin -> 5V, other outer pin -> GND, wiper (middle) -> A0
//   5 LEDs on pins 4-8, each through a 220 ohm resistor to GND
//     pin 4 lights at 1 V, pin 5 at 2 V, pin 6 at 3 V, pin 7 at 4 V, pin 8 at 5 V
// The sketch behaves like a small voltmeter: the exact value is printed and the
// LEDs show the level the way the bar scale on a multimeter does.

const int POT_PIN = A0;            // Test voltage is read on analog pin A0
const int LED_PIN_START = 4;       // First threshold LED
const int LED_COUNT = 5;           // One LED for each whole volt from 1 V to 5 V
const float ADC_MAX = 1023.0;      // Maximum value returned by the 10-bit ADC
const float REFERENCE_VOLTAGE = 5.0;  // The Uno measures against its 5 V supply
const unsigned long SAMPLE_MS = 250;  // Pause between readings

// Lights the threshold LEDs for the measured voltage
void updateThresholdLeds(float voltage) {
  for (int i = 0; i < LED_COUNT; i++) {
    float threshold = i + 1;   // 1 V, 2 V, 3 V, 4 V, 5 V
    if (voltage >= threshold) {
      digitalWrite(LED_PIN_START + i, HIGH);
    } else {
      digitalWrite(LED_PIN_START + i, LOW);
    }
  }
}

void setup() {
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PIN_START + i, OUTPUT);
    digitalWrite(LED_PIN_START + i, LOW);
  }

  Serial.begin(9600);
  Serial.println("=== Analog Voltage Indicator ===");
  Serial.println("Turn the potentiometer to sweep the voltage from 0.00 V to 5.00 V.");
}

void loop() {
  int adcValue = analogRead(POT_PIN);   // Raw reading: 0 to 1023

  // Convert the raw reading into a voltage between 0.00 V and 5.00 V
  float voltage = adcValue * (REFERENCE_VOLTAGE / ADC_MAX);

  Serial.print("Voltage: ");
  Serial.print(voltage, 2);   // The 2 means "print with 2 decimal places"
  Serial.println(" V");

  updateThresholdLeds(voltage);

  delay(SAMPLE_MS);
}
