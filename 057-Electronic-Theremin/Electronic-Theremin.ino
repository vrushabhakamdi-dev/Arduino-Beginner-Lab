// 057 - Electronic Theremin
// A light-controlled musical instrument. Moving your hand over the light
// dependent resistor (LDR) changes the amount of light it receives, and the
// Arduino turns that into a continuous pitch on a passive buzzer - exactly like
// the famous wireless theremin played without touching anything.
// Board: Arduino Uno
//
// Wiring:
//   Light dependent resistor (LDR) + 10k ohm resistor form a voltage divider:
//     +5V -> LDR -> analog pin A0 -> 10k ohm resistor -> GND
//   Passive buzzer (+) -> pin 8, buzzer (-) -> GND
//
// How to play: cover the LDR with your hand to make the pitch fall, move your
// hand away to let more light in and the pitch rises. Slow, small movements
// give smooth slides (portamento); quick movements give sharp jumps.

// ------------------------------ Pin constants ------------------------------
const byte LDR_PIN = A0;     // voltage divider output from the LDR
const byte BUZZER_PIN = 8;   // passive buzzer that can play any frequency

// ------------------------------ Tone constants -----------------------------
const int MIN_FREQ_HZ = 200;    // lowest playable note
const int MAX_FREQ_HZ = 4000;   // highest playable note
const byte SAMPLE_COUNT = 10;   // how many readings are averaged for smoothing
const int FREQ_STEP_HZ = 5;     // pitch is rounded to this size, kills jitter
const unsigned long PRINT_MS = 300;  // Serial refresh rate for the pitch display

// ------------------------------ Smoothing state ----------------------------
int samples[SAMPLE_COUNT];   // rolling buffer of the last ADC readings
byte sampleIndex = 0;        // next position to write in the buffer
bool bufferFilled = false;   // true once the whole buffer holds real readings
int lastPrintedFreq = 0;     // used to only print when the pitch changes
unsigned long lastPrintAt = 0;  // last time the Serial line was refreshed

// Average the newest SAMPLE_COUNT readings to remove flicker and noise
int smoothReading(int raw) {
  samples[sampleIndex] = raw;
  sampleIndex++;
  if (sampleIndex >= SAMPLE_COUNT) {
    sampleIndex = 0;
    bufferFilled = true;
  }

  byte count = bufferFilled ? SAMPLE_COUNT : sampleIndex;  // fewer samples at start-up
  if (count == 0) return raw;

  long sum = 0;
  for (byte i = 0; i < count; i++) sum += samples[i];
  return (int)(sum / count);
}

void setup() {
  pinMode(LDR_PIN, INPUT);       // analog pins are inputs by default, stated for clarity
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.begin(9600);
  Serial.println("=== Electronic Theremin ===");
  Serial.println("Move your hand over the LDR to change the pitch.");
  Serial.println("Reading LDR on A0, tone on pin 8.");
}

void loop() {
  unsigned long now = millis();

  // 1. Read the sensor: more light on the LDR -> lower resistance -> higher voltage
  int raw = analogRead(LDR_PIN);

  // 2. Smooth the reading so the tone does not wobble
  int average = smoothReading(raw);

  // 3. Convert the 0-1023 ADC value into a musical frequency
  int frequency = map(average, 0, 1023, MIN_FREQ_HZ, MAX_FREQ_HZ);

  // 4. Round to FREQ_STEP_HZ steps so tiny hand tremors do not change the note
  frequency = (frequency / FREQ_STEP_HZ) * FREQ_STEP_HZ;
  if (frequency < MIN_FREQ_HZ) frequency = MIN_FREQ_HZ;
  if (frequency > MAX_FREQ_HZ) frequency = MAX_FREQ_HZ;

  // 5. Send the frequency to the passive buzzer (pin 8 works with any tone pin)
  tone(BUZZER_PIN, frequency);

  // 6. Report the current pitch so you can see the value you are playing
  if (frequency != lastPrintedFreq && (now - lastPrintAt) >= PRINT_MS) {
    lastPrintedFreq = frequency;
    lastPrintAt = now;
    Serial.print("Frequency: ");
    Serial.print(frequency);
    Serial.println(" Hz");
  }

  delay(20);  // short pause keeps the readings steady
}
