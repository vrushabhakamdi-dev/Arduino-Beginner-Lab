// 011 - Reaction Time Tester
// Board: Arduino Uno
// Wiring:
//   LED              -> pin 13 through a 220 ohm resistor (the built-in LED also works)
//   Push button      -> one leg to pin 2, other leg to GND (internal pull-up is used)
//   Passive buzzer   -> (+) to pin 8, (-) to GND
// Open the Serial Monitor at 9600 baud, wait for the LED, then press the button
// as fast as you can. Pressing before the LED lights counts as a false start.

// ---------------- Pin constants ----------------
const int LED_PIN = 13;      // LED that starts each round
const int BUTTON_PIN = 2;    // Reaction button (INPUT_PULLUP, wired to GND)
const int BUZZER_PIN = 8;    // Passive buzzer used for the "too early" error

// ---------------- Timing constants ----------------
const unsigned long MIN_WAIT_MS = 1000;   // Shortest random wait: 1 second
const unsigned long MAX_WAIT_MS = 4000;   // Longest random wait: 4 seconds
const unsigned long DEBOUNCE_MS = 30;     // Press must stay stable this long
const unsigned int ERROR_TONE_HZ = 150;   // Low buzz for a false start
const unsigned int ERROR_TONE_MS = 300;   // Length of the error buzz
const unsigned long ROUND_PAUSE_MS = 300; // Pause before the next round

// ---------------- Score tracking ----------------
unsigned long bestScoreMs = 0;  // 0 means no valid score has been recorded yet

// Plays a short low buzz so the player knows the press was too early
void errorSignal() {
  tone(BUZZER_PIN, ERROR_TONE_HZ);  // tone() makes the passive buzzer hum
  delay(ERROR_TONE_MS);
  noTone(BUZZER_PIN);               // Stop the tone
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Internal pull-up keeps the pin HIGH until pressed
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);

  // Seed the random generator once, otherwise every reset gives the same delays.
  // A0 is left floating, so it picks up electrical noise to use as the seed.
  randomSeed(analogRead(A0));

  Serial.println("=== Reaction Time Tester ===");
  Serial.println("Wait for the LED, then press the button as fast as you can!");
  Serial.println("Pressing too early is an error and sounds the buzzer.");
}

void loop() {
  digitalWrite(LED_PIN, LOW);

  // Make sure the button is released before a new round starts,
  // otherwise holding the button would look like a false start.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);  // Nothing else needs to run while we wait for the release
  }

  Serial.println();
  Serial.println("Get ready... wait for the LED!");

  // Pick a random waiting time between 1 and 4 seconds.
  // random(min, max) stops one number before max, so MAX_WAIT_MS + 1 is used.
  unsigned long waitMs = random(MIN_WAIT_MS, MAX_WAIT_MS + 1);
  unsigned long waitStart = millis();

  // Phase 1: wait in silence. Any press during this phase is a false start.
  bool earlyPress = false;
  while (millis() - waitStart < waitMs) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      earlyPress = true;
      break;
    }
  }

  if (earlyPress) {
    errorSignal();
    Serial.println("TOO EARLY! Press only after the LED lights up.");
    return;  // loop() starts again and waits for the button to be released
  }

  // Phase 2: the LED turns ON and the reaction timer starts right now
  digitalWrite(LED_PIN, HIGH);
  unsigned long startMs = millis();
  unsigned long pressedAt = 0;

  while (pressedAt == 0) {           // Wait until the button is really pressed
    if (digitalRead(BUTTON_PIN) == LOW) {
      unsigned long firstContact = millis();
      delay(DEBOUNCE_MS);            // Ignore contact bounce
      if (digitalRead(BUTTON_PIN) == LOW) {
        pressedAt = firstContact;    // Use the first contact time, not the delayed one
      }
    }
  }

  unsigned long reactionMs = pressedAt - startMs;  // millis() based measurement
  digitalWrite(LED_PIN, LOW);

  // Phase 3: show the result and keep track of the best round
  Serial.print("Reaction time: ");
  Serial.print(reactionMs);
  Serial.println(" ms");

  if (bestScoreMs == 0 || reactionMs < bestScoreMs) {
    bestScoreMs = reactionMs;
    Serial.println("*** NEW BEST SCORE! ***");
  }

  Serial.print("Best score so far: ");
  Serial.print(bestScoreMs);
  Serial.println(" ms");

  delay(ROUND_PAUSE_MS);
}
