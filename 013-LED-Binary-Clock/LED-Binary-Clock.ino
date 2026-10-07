// 013 - LED Binary Clock
// Board: Arduino Uno
// Wiring: 11 LEDs, each through a 220 ohm resistor to GND
//   HOURS   (5 LEDs): pin 2 = value  1, pin 3 = 2, pin 4 = 4, pin 5 = 8, pin 6 = 16
//   MINUTES (6 LEDs): pin 7 = value  1, pin 8 = 2, pin 9 = 4, pin 10 = 8,
//                     pin 11 = 16, pin 12 = 32
// To read the clock, add up the values of the lit LEDs in each group.

// ---------------- LED group constants ----------------
const int HOUR_PIN_START = 2;   // First hours LED (5 LEDs on pins 2-6)
const int HOUR_BITS = 5;        // 5 bits show 0-31, the clock only needs 0-23
const int MIN_PIN_START = 7;    // First minutes LED (6 LEDs on pins 7-12)
const int MIN_BITS = 6;         // 6 bits show 0-63, the clock only needs 0-59
const int LED_PIN_END = 12;     // Last LED pin used by the clock

// ---------------- Start time - change these to set your clock ----------------
const int START_HOUR = 12;
const int START_MINUTE = 30;

const unsigned long MS_PER_MINUTE = 60000UL;

unsigned long bootMillis = 0;   // millis() reading when the sketch started
int lastShownMinute = -1;       // So the Serial Monitor only prints on minute change

// Lights the LEDs that hold the bits of "value" (bit 0 goes to startPin)
void showBits(int value, int startPin, int bitCount) {
  for (int i = 0; i < bitCount; i++) {
    int bitIsOn = (value >> i) & 1;         // Read one bit of the value
    digitalWrite(startPin + i, bitIsOn ? HIGH : LOW);
  }
}

// Prints a number with a leading zero, e.g. 7 becomes 07
void printTwoDigits(int value) {
  if (value < 10) Serial.print('0');
  Serial.print(value);
}

// Prints a number as binary digits, e.g. 5 becomes 00101
void printBinary(int value, int bitCount) {
  for (int i = bitCount - 1; i >= 0; i--) {
    Serial.print((value >> i) & 1);
  }
}

void setup() {
  for (int pin = HOUR_PIN_START; pin <= LED_PIN_END; pin++) {
    pinMode(pin, OUTPUT);   // All 11 LEDs are outputs (pins 2 to 12)
    digitalWrite(pin, LOW);
  }

  Serial.begin(9600);
  Serial.println("=== LED Binary Clock ===");
  Serial.print("Starting at ");
  printTwoDigits(START_HOUR);
  Serial.print(':');
  printTwoDigits(START_MINUTE);
  Serial.println("  (hour LEDs: pins 2-6, minute LEDs: pins 7-12)");
  Serial.println("Tip: add a DS3231 RTC module later for accurate time that survives a power cut.");

  bootMillis = millis();   // Remember when the clock was started
}

void loop() {
  // millis() counts the milliseconds since the board started running,
  // so elapsed time plus the start time gives the current time.
  unsigned long elapsed = millis() - bootMillis;
  unsigned long totalMinutes = (unsigned long)START_HOUR * 60UL + START_MINUTE
                               + elapsed / MS_PER_MINUTE;

  int hour = (totalMinutes / 60) % 24;   // Wrap back to 0 after 23:59
  int minute = totalMinutes % 60;        // Wrap back to 0 after 59 minutes

  // Send the bits of each value to its group of LEDs
  showBits(hour, HOUR_PIN_START, HOUR_BITS);
  showBits(minute, MIN_PIN_START, MIN_BITS);

  // Only print when the minute actually changes, so the monitor stays readable
  if (minute != lastShownMinute) {
    lastShownMinute = minute;
    Serial.print("Time: ");
    printTwoDigits(hour);
    Serial.print(':');
    printTwoDigits(minute);
    Serial.print("   hours binary: ");
    printBinary(hour, HOUR_BITS);
    Serial.print("   minutes binary: ");
    printBinary(minute, MIN_BITS);
    Serial.println();
  }

  // UPGRADE NOTE: millis() only counts while the Uno has power and it drifts
  // a little over time. A DS3231 RTC module keeps the real time accurately
  // and keeps running on its coin cell when the board is switched off.
}
