// 060 - Adjustable Buzzer Tone Generator
// Turn the potentiometer to sweep the pitch of a passive buzzer from 100 Hz up
// to 5000 Hz, and press the button to switch the tone on or off. The current
// frequency is printed to the Serial Monitor so you can see what you hear.
// Board: Arduino Uno
//
// Wiring:
//   Potentiometer: one outer leg -> +5V, other outer leg -> GND,
//                  wiper (middle leg) -> analog pin A0
//   Passive buzzer (+) -> pin 8, buzzer (-) -> GND
//   On/off button -> pin 2 (shorts the pin to GND, internal pull-up used)
//
// Waveform note: Arduino tone() can only produce a SQUARE wave. For other
// waveforms you can send the pin through an RC low-pass filter to smooth the
// square into something closer to a sine, drive a transistor from a faster PWM
// signal and average it with a capacitor to approximate a sawtooth, or add an
// external waveform generator module that accepts a digital or serial command.

// ------------------------------ Pin constants ------------------------------
const byte POT_PIN = A0;      // potentiometer wiper sets the frequency
const byte BUZZER_PIN = 8;    // passive buzzer output
const byte ON_OFF_PIN = 2;    // button toggles the tone on and off

// ------------------------------ Tone constants -----------------------------
const int MIN_FREQ_HZ = 100;     // lowest frequency the pot can select
const int MAX_FREQ_HZ = 5000;    // highest frequency the pot can select
const int FREQ_STEP_HZ = 10;     // frequency is rounded to this size
const unsigned long DEBOUNCE_MS = 30;
const unsigned long PRINT_MS = 250;  // how often the Serial line is refreshed

// ------------------------------ Runtime state ------------------------------
bool toneOn = true;         // whether the buzzer is sounding
int currentFreq = MIN_FREQ_HZ;

// Button debounce state
bool buttonRaw;
bool buttonStable;
unsigned long buttonLastChange;

// Last time the status was printed
unsigned long lastPrintAt = 0;

// Returns true once, on the moment the button is pressed
bool buttonPressed(unsigned long now) {
  bool reading = digitalRead(ON_OFF_PIN);  // LOW = pressed (internal pull-up)

  if (reading != buttonRaw) {
    buttonRaw = reading;
    buttonLastChange = now;
  }

  if ((now - buttonLastChange) >= DEBOUNCE_MS && reading != buttonStable) {
    buttonStable = reading;
    if (buttonStable == LOW) return true;
  }
  return false;
}

// Reads the potentiometer and converts it into a clean frequency value
int readFrequency() {
  int potValue = analogRead(POT_PIN);                       // 0 - 1023
  int frequency = map(potValue, 0, 1023, MIN_FREQ_HZ, MAX_FREQ_HZ);

  // Round to FREQ_STEP_HZ so the pitch does not tremble on tiny pot movements
  frequency = (frequency / FREQ_STEP_HZ) * FREQ_STEP_HZ;
  if (frequency < MIN_FREQ_HZ) frequency = MIN_FREQ_HZ;
  if (frequency > MAX_FREQ_HZ) frequency = MAX_FREQ_HZ;
  return frequency;
}

void setup() {
  pinMode(POT_PIN, INPUT);
  pinMode(ON_OFF_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  buttonRaw = digitalRead(ON_OFF_PIN);
  buttonStable = buttonRaw;
  buttonLastChange = millis();

  Serial.begin(9600);
  Serial.println("=== Adjustable Buzzer Tone Generator ===");
  Serial.println("Turn the potentiometer to change the frequency.");
  Serial.println("Press the button on pin 2 to switch the tone on or off.");
}

void loop() {
  unsigned long now = millis();

  // --- On / off button toggles the output ---
  if (buttonPressed(now)) {
    toneOn = !toneOn;
    if (toneOn) {
      tone(BUZZER_PIN, currentFreq);
      Serial.println(">>> Tone ON");
    } else {
      noTone(BUZZER_PIN);
      Serial.println(">>> Tone OFF");
    }
    lastPrintAt = now;
  }

  // --- Potentiometer always sets the frequency ---
  int frequency = readFrequency();

  if (frequency != currentFreq) {  // only restart the tone when the value really moved
    currentFreq = frequency;
    if (toneOn) {
      tone(BUZZER_PIN, currentFreq);  // restart with the new frequency
    }
  }

  // --- Report the status a few times per second ---
  if (now - lastPrintAt >= PRINT_MS) {
    lastPrintAt = now;
    Serial.print(toneOn ? "ON  " : "OFF ");
    Serial.print(" Frequency: ");
    Serial.print(currentFreq);
    Serial.println(" Hz");
  }
}
