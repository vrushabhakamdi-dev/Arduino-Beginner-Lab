// 058 - Mini Electronic Piano
// Eight push buttons play the eight notes of a C major scale (C4 to C5) through
// a passive buzzer. Hold a button and the note sounds; let go and it stops.
// Every note name is printed in the Serial Monitor as you play.
// Board: Arduino Uno
//
// Wiring:
//   8 buttons -> pins 2, 3, 4, 5, 6, 7, 8 and 9
//     (each button shorts its pin to GND; internal pull-ups are enabled)
//   Passive buzzer (+) -> pin 10, buzzer (-) -> GND
//
// Button order (left to right):  C4  D4  E4  F4  G4  A4  B4  C5

// ------------------------------ Pin constants ------------------------------
const byte NOTE_COUNT = 8;
const byte BUTTON_PIN[NOTE_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9};
const byte BUZZER_PIN = 10;  // passive buzzer on a pin that is not used by a button

// ------------------------------ Note table ---------------------------------
// Standard frequencies in Hz for one octave of a C major scale
const int NOTE_FREQ[NOTE_COUNT] = {262, 294, 330, 349, 392, 440, 494, 523};
const char *NOTE_NAME[NOTE_COUNT] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};

// ------------------------------ Timing constants ---------------------------
const unsigned long DEBOUNCE_MS = 30;

// ------------------------------ Runtime state ------------------------------
int activeNote = -1;                 // index of the note sounding now, -1 = silence
bool buttonRaw[NOTE_COUNT];
bool buttonStable[NOTE_COUNT];
unsigned long buttonLastChange[NOTE_COUNT];

// Updates every button with debouncing and returns the index of the button
// that is currently held down, or -1 if no button is pressed.
int readHeldButton(unsigned long now) {
  int held = -1;

  for (byte i = 0; i < NOTE_COUNT; i++) {
    bool reading = digitalRead(BUTTON_PIN[i]);  // LOW = pressed

    if (reading != buttonRaw[i]) {       // raw change, restart the debounce timer
      buttonRaw[i] = reading;
      buttonLastChange[i] = now;
    }

    if ((now - buttonLastChange[i]) >= DEBOUNCE_MS && reading != buttonStable[i]) {
      buttonStable[i] = reading;         // the reading has settled
    }

    if (buttonStable[i] == LOW && held == -1) held = i;
  }
  return held;
}

void setup() {
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  for (byte i = 0; i < NOTE_COUNT; i++) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);  // pull-up: pressing the button reads LOW
    buttonRaw[i] = digitalRead(BUTTON_PIN[i]);
    buttonStable[i] = buttonRaw[i];
    buttonLastChange[i] = millis();
  }

  Serial.println("=== Mini Electronic Piano ===");
  Serial.println("Buttons on pins 2-9 play C4, D4, E4, F4, G4, A4, B4, C5.");
  Serial.println("Hold a button to hear the note, release it to stop.");
}

void loop() {
  int held = readHeldButton(millis());

  if (held >= 0 && held != activeNote) {
    // A new note started: stop the old one and play the new pitch
    activeNote = held;
    tone(BUZZER_PIN, NOTE_FREQ[activeNote]);
    Serial.print("Playing: ");
    Serial.print(NOTE_NAME[activeNote]);
    Serial.print("  (");
    Serial.print(NOTE_FREQ[activeNote]);
    Serial.println(" Hz)");
  } else if (held < 0 && activeNote != -1) {
    // The key was released: silence the buzzer
    noTone(BUZZER_PIN);
    activeNote = -1;
    Serial.println("Released - silence");
  }

  // If the same button stays held, nothing changes and the tone keeps ringing.
  // loop() repeats continuously, so presses and releases are caught instantly.
}
