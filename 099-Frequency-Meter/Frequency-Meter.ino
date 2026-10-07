/*
 * 099 - Frequency Meter
 * Board: Arduino Uno
 *
 * Measures the frequency of a digital square-wave signal fed into pin 2.
 * Pin 2 is INT0, the hardware interrupt pin of the ATmega328P, so every
 * rising edge is counted by a tiny interrupt service routine that never
 * misses pulses - even while the main program is busy printing.
 *
 * Every second (a "gate window" timed with millis()) the sketch:
 *   - takes a snapshot of the pulse counter and resets it,
 *   - calculates frequency = pulses / gate time, printed in Hz,
 *   - calculates period   = 1 / frequency, printed in microseconds,
 *   - prints both values to the Serial Monitor at 9600 baud.
 *
 * INPUT VOLTAGE - IMPORTANT:
 *   The signal must stay between 0 V and 5 V. The Uno's pins are NOT 5 V
 *   tolerant above 5 V and are not meant for negative voltages. Feed
 *   anything larger (for example a 12 V signal) through a resistor divider
 *   that scales it down to a maximum of 5 V, and AC signals must be biased
 *   or clamped first.
 *
 * GOOD TEST SIGNALS:
 *   - tone() on another Arduino: connect pin 9 of a second Uno (tone pin)
 *     to pin 2 of this one and try 100, 1000 and 5000 Hz.
 *   - A function generator set to a 0-5 V square wave.
 *   - A 555 timer astable circuit or a PWM output (analogWrite).
 *
 * No external library is required - this project uses only core Arduino
 * functions and attachInterrupt().
 */

// ---------------- Named pin constants ----------------
const int SIGNAL_PIN = 2;   // INT0 - must be pin 2 (or pin 3 for INT1)
const unsigned long GATE_MS = 1000;   // gate window length in milliseconds

// The counter is modified inside an interrupt, so it must be volatile.
volatile unsigned long pulseCount = 0;
unsigned long windowStartMs = 0;   // when the current gate window began
unsigned long totalPulses = 0;     // pulses seen since power-up
unsigned long windowNumber = 0;    // how many measurements have been taken

// Interrupt service routine: runs on every rising edge of the input.
// Keep ISRs short - counting one variable is all it does.
void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);

  pinMode(SIGNAL_PIN, INPUT);   // 0 V to 5 V only - see the header note

  // digitalPinToInterrupt() makes the code portable (pin 2 = INT0 on Uno).
  attachInterrupt(digitalPinToInterrupt(SIGNAL_PIN), countPulse, RISING);

  windowStartMs = millis();

  Serial.println("Frequency Meter ready - signal into pin 2 (INT0).");
  Serial.println("Maximum input 5 V. Test with tone() on pin 9 of a 2nd Arduino.");
  Serial.println("Measuring over 1 second gate windows...");
  Serial.println("-----------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // ---- One full gate window has passed: take a measurement ----
  if (now - windowStartMs >= GATE_MS) {
    // Use the exact elapsed time so rounding of the window does not skew
    // the result (the print statements above can steal a few milliseconds).
    unsigned long elapsedMs = now - windowStartMs;
    windowStartMs = now;

    // Copy and clear the counter atomically: the interrupt must not add a
    // pulse halfway through the read.
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();

    windowNumber++;
    totalPulses += count;

    // Frequency = pulses divided by the window length in seconds.
    double frequencyHz = (double)count * 1000.0 / (double)elapsedMs;

    Serial.print("Gate #");
    Serial.print(windowNumber);
    Serial.print(" | Pulses: ");
    Serial.print(count);

    if (count == 0) {
      // Nothing arrived during the whole second - say so instead of
      // dividing by zero and printing nonsense.
      Serial.println(" | Frequency: 0.00 Hz (no signal on pin 2)");
    } else {
      double periodUs = 1000000.0 / frequencyHz;   // period = 1 / frequency

      Serial.print(" | Frequency: ");
      Serial.print(frequencyHz, 2);
      Serial.print(" Hz | Period: ");
      Serial.print(periodUs, 2);
      Serial.println(" us");
    }

    Serial.print("    Elapsed: ");
    Serial.print(elapsedMs);
    Serial.print(" ms | total pulses: ");
    Serial.println(totalPulses);
  }

  delay(5);   // the interrupt does the counting, so this delay is harmless
}
