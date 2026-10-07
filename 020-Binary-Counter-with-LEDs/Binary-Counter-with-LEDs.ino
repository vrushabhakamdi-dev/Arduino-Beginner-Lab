// 020 - Binary Counter with LEDs
// Board: Arduino Uno
// Wiring: 8 LEDs on pins 2-9, each through a 220 ohm resistor to GND.
//
// Bit-to-pin mapping (least significant bit first):
//   pin 2 = bit 0 -> value   1      pin 6 = bit 4 -> value  16
//   pin 3 = bit 1 -> value   2      pin 7 = bit 5 -> value  32
//   pin 4 = bit 2 -> value   4      pin 8 = bit 6 -> value  64
//   pin 5 = bit 3 -> value   8      pin 9 = bit 7 -> value 128
//
// So 0 lights no LED, 255 lights all eight LEDs, and every value in between
// is shown as the eight-bit binary pattern of that number.

const int LED_PIN_START = 2;         // First LED (bit 0)
const int LED_COUNT = 8;             // Eight LEDs make one byte
const unsigned long COUNT_DELAY_MS = 300;  // Pause between two numbers

// Lights the LEDs so the eight bits of "value" become visible
void showValue(int value) {
  for (int i = 0; i < LED_COUNT; i++) {
    // (value >> i) shifts bit i down to the lowest position, & 1 keeps only it
    int bitIsOn = (value >> i) & 1;
    digitalWrite(LED_PIN_START + i, bitIsOn ? HIGH : LOW);
  }
}

// Prints the number as 8 binary digits, for example 5 becomes 00000101
void printBinary(int value) {
  for (int i = LED_COUNT - 1; i >= 0; i--) {  // Print the highest bit first
    Serial.print((value >> i) & 1);
  }
}

void setup() {
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PIN_START + i, OUTPUT);
    digitalWrite(LED_PIN_START + i, LOW);
  }

  Serial.begin(9600);
  Serial.println("=== Binary Counter 0 - 255 ===");
  Serial.println("Decimal    Binary");
}

void loop() {
  // Count through every 8-bit value: 0, 1, 2, ... 254, 255, then start again
  for (int number = 0; number <= 255; number++) {
    showValue(number);

    Serial.print("  ");
    if (number < 100) Serial.print(' ');  // Keep the decimal column aligned
    if (number < 10) Serial.print(' ');
    Serial.print(number);
    Serial.print("     ");
    printBinary(number);
    Serial.println();

    delay(COUNT_DELAY_MS);  // Slow enough to watch each value on the LEDs
  }

  Serial.println("--- wrapped back to 0 ---");
}
