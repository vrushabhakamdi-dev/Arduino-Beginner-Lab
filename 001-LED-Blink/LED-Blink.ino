// 001 - LED Blink
// The classic "Hello World" of Arduino: blink an LED on and off every second.
// Board: Arduino Uno
// Wiring: LED anode (+) -> pin 13 through a 220 ohm resistor, LED cathode (-) -> GND

// Pin and timing constants (named constants instead of magic numbers)
const int LED_PIN = 13;         // LED is connected to digital pin 13 (built-in LED on Uno)
const unsigned long BLINK_MS = 1000;  // 1000 milliseconds = 1 second

void setup() {
  // Configure the LED pin as an output so we can turn the LED on and off
  pinMode(LED_PIN, OUTPUT);

  // Start serial communication so we can watch the state in the Serial Monitor
  Serial.begin(9600);
  Serial.println("LED Blink starting...");
}

void loop() {
  // Turn the LED ON by sending 5V to the pin
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(BLINK_MS);  // Wait 1 second while the LED stays on

  // Turn the LED OFF by pulling the pin to 0V
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(BLINK_MS);  // Wait 1 second while the LED stays off

  // loop() runs again automatically, so the blinking continues forever
}
