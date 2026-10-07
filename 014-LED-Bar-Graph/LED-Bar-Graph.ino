// 014 - LED Bar Graph
// Board: Arduino Uno
// Wiring:
//   Potentiometer: one outer pin -> 5V, other outer pin -> GND, wiper (middle) -> A0
//   10 LEDs on pins 2-11, each through a 220 ohm resistor to GND
// Turn the potentiometer knob and the bar of LEDs grows and shrinks like a
// level meter while the Serial Monitor shows the value as a percentage.

const int POT_PIN = A0;          // Potentiometer wiper is on analog pin A0
const int LED_PIN_START = 2;     // First LED of the bar
const int LED_COUNT = 10;        // LEDs on pins 2,3,4,5,6,7,8,9,10,11
const unsigned long SAMPLE_MS = 100;  // Pause between readings (keeps output readable)

void setup() {
  // Configure every LED pin as an output using a loop and the base pin number
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PIN_START + i, OUTPUT);
    digitalWrite(LED_PIN_START + i, LOW);
  }

  Serial.begin(9600);
  Serial.println("=== LED Bar Graph ===");
  Serial.println("Turn the potentiometer to move the bar from 0% to 100%.");
}

void loop() {
  int sensorValue = analogRead(POT_PIN);              // 0 (fully down) to 1023 (fully up)

  // map() rescales one range into another:
  // 0-1023 becomes 0-10 lit LEDs and also 0-100 percent
  int litLeds = map(sensorValue, 0, 1023, 0, LED_COUNT);
  int percent = map(sensorValue, 0, 1023, 0, 100);

  // Light the first "litLeds" LEDs and switch the rest off
  for (int i = 0; i < LED_COUNT; i++) {
    if (i < litLeds) {
      digitalWrite(LED_PIN_START + i, HIGH);
    } else {
      digitalWrite(LED_PIN_START + i, LOW);
    }
  }

  // Show the raw reading, the bar level and the percentage
  Serial.print("Potentiometer: ");
  Serial.print(sensorValue);
  Serial.print("   Level: ");
  Serial.print(litLeds);
  Serial.print('/');
  Serial.print(LED_COUNT);
  Serial.print("   Percent: ");
  Serial.print(percent);
  Serial.println('%');

  delay(SAMPLE_MS);
}
