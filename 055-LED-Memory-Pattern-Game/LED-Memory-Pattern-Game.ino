// 055 - LED Memory Pattern Game
// A Simon-style memory game. The Arduino shows a pattern of flashing LEDs with
// matching tones, one step longer every round, and you must repeat it exactly.
// Get it wrong and a losing jingle plays; every correct repeat adds a point.
// Board: Arduino Uno
//
// Wiring:
//   4 LEDs -> pins 4, 5, 6 and 7 (anode to the pin through a 220 ohm resistor,
//             cathode to GND)
//   4 buttons -> pins 8, 9, 10 and 11 (each button shorts its pin to GND,
//             internal pull-ups are enabled)
//   Passive buzzer (+) -> pin 12, buzzer (-) -> GND
//
// Each LED has its own tone, so you can also play the game by sound alone.

// ------------------------------ Pin constants ------------------------------
const byte STEP_COUNT = 4;                              // number of different buttons/LEDs
const byte LED_PIN[STEP_COUNT] = {4, 5, 6, 7};          // the four indicator LEDs
const byte BUTTON_PIN[STEP_COUNT] = {8, 9, 10, 11};     // the four answer buttons
const byte BUZZER_PIN = 12;                             // passive buzzer for the tones

// ------------------------------ Game constants -----------------------------
const int TONE_HZ[STEP_COUNT] = {262, 330, 392, 494};   // C4, D4, G4, B4 - one per step
const byte MAX_LEVEL = 32;                              // pattern array size / target length
const unsigned long DEBOUNCE_MS = 30;
const unsigned long STEP_FLASH_MS = 350;                // LED on time while showing the pattern
const unsigned long STEP_GAP_MS = 150;                  // gap between two pattern steps

// ------------------------------ Game state ---------------------------------
byte sequence[MAX_LEVEL];  // the pattern to remember, grows by one every round
byte level = 0;            // how many steps are in the current pattern
byte inputIndex = 0;       // how many steps the player has entered correctly so far
bool acceptingInput = false; // true while it is the player's turn

// Button debounce state
bool buttonRaw[STEP_COUNT];
bool buttonStable[STEP_COUNT];
unsigned long buttonLastChange[STEP_COUNT];

// Returns the index (0-3) of the button just pressed, or -1 if none
int readButtonPress(unsigned long now) {
  int found = -1;

  for (byte i = 0; i < STEP_COUNT; i++) {
    bool reading = digitalRead(BUTTON_PIN[i]);  // LOW = pressed

    if (reading != buttonRaw[i]) {
      buttonRaw[i] = reading;
      buttonLastChange[i] = now;
    }

    if ((now - buttonLastChange[i]) >= DEBOUNCE_MS && reading != buttonStable[i]) {
      buttonStable[i] = reading;
      if (buttonStable[i] == LOW && found == -1) found = i;
    }
  }
  return found;
}

// Lights one LED and plays its tone for a moment
void flashStep(byte index, unsigned long onMs) {
  digitalWrite(LED_PIN[index], HIGH);
  tone(BUZZER_PIN, TONE_HZ[index]);
  delay(onMs);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN[index], LOW);
}

// Replays the whole pattern so far for the player to watch
void showPattern() {
  Serial.println("Watch the pattern...");
  delay(400);

  for (byte i = 0; i < level; i++) {
    flashStep(sequence[i], STEP_FLASH_MS);
    delay(STEP_GAP_MS);
  }

  Serial.println("Your turn - repeat it!");
}

void addToSequence() {
  sequence[level] = random(0, STEP_COUNT);  // pick a new random step
  level++;
  inputIndex = 0;
  showPattern();
  acceptingInput = true;
}

void loseJingle() {
  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, 392, 200);
  delay(250);
  tone(BUZZER_PIN, 311, 200);
  delay(250);
  tone(BUZZER_PIN, 233, 400);  // falling pitch tells you the game is over
  delay(500);
  noTone(BUZZER_PIN);
}

void gameOver() {
  acceptingInput = false;

  // Flash every LED so the failure is obvious
  for (byte i = 0; i < STEP_COUNT; i++) digitalWrite(LED_PIN[i], HIGH);
  loseJingle();
  for (byte i = 0; i < STEP_COUNT; i++) digitalWrite(LED_PIN[i], LOW);

  Serial.println("GAME OVER - wrong step!");
  Serial.print("You reached level ");
  Serial.print(level);
  Serial.print(" with a score of ");
  Serial.print(level - 1);
  Serial.println(" points.");
}

void resetGame() {
  level = 0;
  inputIndex = 0;
  acceptingInput = false;
  for (byte i = 0; i < STEP_COUNT; i++) digitalWrite(LED_PIN[i], LOW);

  Serial.println();
  Serial.println("--- New game ---");
  Serial.println("Get ready...");
  delay(800);
  addToSequence();  // level becomes 1 and the first step is shown
}

void handlePlayerPress(byte index) {
  // Echo the player's choice: flash the LED and play the tone
  flashStep(index, 200);

  if (index != sequence[inputIndex]) {
    gameOver();
    return;
  }

  inputIndex++;

  // The whole pattern has been repeated correctly
  if (inputIndex == level) {
    acceptingInput = false;
    Serial.print("Correct! Level ");
    Serial.print(level);
    Serial.print(" complete - score ");
    Serial.println(level);
    delay(500);

    if (level >= MAX_LEVEL) {
      Serial.println("MAXIMUM LEVEL REACHED - you are a memory champion!");
      resetGame();
    } else {
      addToSequence();  // pattern grows by one step
    }
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));  // seed from noise on A0 so every game differs

  for (byte i = 0; i < STEP_COUNT; i++) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
    buttonRaw[i] = digitalRead(BUTTON_PIN[i]);
    buttonStable[i] = buttonRaw[i];
    buttonLastChange[i] = millis();
  }

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.println("=== LED Memory Pattern Game ===");
  Serial.println("Repeat the flashing pattern. It grows one step every round.");

  resetGame();
}

void loop() {
  if (!acceptingInput) return;  // the Arduino is busy showing the pattern

  int pressed = readButtonPress(millis());
  if (pressed >= 0) {
    handlePlayerPress((byte)pressed);
  }
}
