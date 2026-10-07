// 005 - Buzzer Doorbell
// Press the button and a passive buzzer plays a two-note "ding-dong"
// doorbell chime (523 Hz then 392 Hz). The doorbell will not fire again
// until the button is released, so one press gives one chime.
// Board: Arduino Uno
// Wiring: Push button between pin 2 and GND (internal pull-up used).
//         PASSIVE buzzer (+) to pin 8, buzzer (-) to GND.

// Pin and sound constants
const int BUTTON_PIN = 2;         // Push button input
const int BUZZER_PIN = 8;         // Passive buzzer output (must be a PWM/tone capable pin)
const int NOTE_1 = 523;           // "ding" note: C5, 523 Hz
const int NOTE_2 = 392;           // "dong" note: G4, 392 Hz
const int NOTE_1_MS = 300;        // Duration of the first note in ms
const int NOTE_2_MS = 500;        // Duration of the second note in ms
const int GAP_MS = 40;            // Tiny silence between the two notes

bool chimePlayed = false;         // Repeat guard: one chime per press

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Button to GND, no external resistor needed
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Buzzer Doorbell ready. Press the button.");
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);

  // LOW means the button is pressed (INPUT_PULLUP wiring)
  if (buttonState == LOW && !chimePlayed) {
    Serial.println("Doorbell: ding-dong!");

    // First note: "ding"
    tone(BUZZER_PIN, NOTE_1);
    delay(NOTE_1_MS);

    // Brief gap so the two notes sound separate
    delay(GAP_MS);

    // Second note: "dong"
    tone(BUZZER_PIN, NOTE_2);
    delay(NOTE_2_MS);

    // Stop the buzzer so it goes silent after the chime
    noTone(BUZZER_PIN);

    // Set the guard so holding the button does not repeat the chime
    chimePlayed = true;
  }

  // Once the button is released, clear the guard so the next press works
  if (buttonState == HIGH) {
    chimePlayed = false;
  }
}
