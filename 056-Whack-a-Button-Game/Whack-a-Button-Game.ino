// 056 - Whack-a-Button Game
// A random LED lights up and you have a shrinking time window to press its
// matching button. Every hit raises your score; miss three times and the game
// is over. The level (and therefore the speed) increases as you score.
// Board: Arduino Uno
//
// Wiring:
//   4 LEDs -> pins 4, 5, 6 and 7 (anode to the pin through a 220 ohm resistor,
//             cathode to GND)
//   4 buttons -> pins 8, 9, 10 and 11 (each button shorts its pin to GND,
//             internal pull-ups are enabled)
//   Passive buzzer (+) -> pin 12, buzzer (-) -> GND

// ------------------------------ Pin constants ------------------------------
const byte TARGET_COUNT = 4;
const byte LED_PIN[TARGET_COUNT] = {4, 5, 6, 7};        // the targets that light up
const byte BUTTON_PIN[TARGET_COUNT] = {8, 9, 10, 11};   // the matching whack buttons
const byte BUZZER_PIN = 12;                             // hit / miss / game-over sounds

// ------------------------------ Game constants -----------------------------
const byte MISSES_TO_END = 3;         // three misses finish the game
const byte HITS_PER_LEVEL = 5;        // every 5 hits the level goes up
const unsigned long START_WINDOW_MS = 1500;  // time allowed for the first target
const unsigned long MIN_WINDOW_MS = 600;     // fastest window, no matter how high the level
const unsigned long WINDOW_STEP_MS = 150;    // window shortens by this much per level
const unsigned long DEBOUNCE_MS = 30;
const int HIT_TONE_HZ = 1200;
const int MISS_TONE_HZ = 200;

// ------------------------------ Game state ---------------------------------
int score = 0;            // hits landed
int level = 1;            // speed level, rises every HITS_PER_LEVEL hits
byte misses = 0;          // misses so far (ends the game at MISSES_TO_END)
int activeTarget = -1;    // which LED is lit (-1 = none)
bool targetActive = false;
unsigned long targetStartAt = 0;
unsigned long targetWindowMs = START_WINDOW_MS;

// Button debounce state
bool buttonRaw[TARGET_COUNT];
bool buttonStable[TARGET_COUNT];
unsigned long buttonLastChange[TARGET_COUNT];

// Returns the index (0-3) of the button just pressed, or -1 if none
int readButtonPress(unsigned long now) {
  int found = -1;

  for (byte i = 0; i < TARGET_COUNT; i++) {
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

void clearTargets() {
  for (byte i = 0; i < TARGET_COUNT; i++) digitalWrite(LED_PIN[i], LOW);
}

// How much time the player gets for the current level
unsigned long windowForLevel() {
  long window = (long)START_WINDOW_MS - (long)(level - 1) * WINDOW_STEP_MS;
  if (window < (long)MIN_WINDOW_MS) window = MIN_WINDOW_MS;
  return (unsigned long)window;
}

void printStatus() {
  Serial.print("Score: ");
  Serial.print(score);
  Serial.print("   Level: ");
  Serial.print(level);
  Serial.print("   Misses: ");
  Serial.print(misses);
  Serial.print("/");
  Serial.print(MISSES_TO_END);
  Serial.print("   Window: ");
  Serial.print(targetWindowMs);
  Serial.println(" ms");
}

void spawnTarget(unsigned long now) {
  byte index = random(0, TARGET_COUNT);

  // Never repeat the same target twice in a row - it feels stuck otherwise
  if (index == (byte)activeTarget) index = (index + 1) % TARGET_COUNT;

  activeTarget = index;
  targetActive = true;
  targetStartAt = now;
  targetWindowMs = windowForLevel();

  clearTargets();
  digitalWrite(LED_PIN[index], HIGH);  // light the new target
}

void gameOver() {
  targetActive = false;
  clearTargets();

  // Descending game-over jingle
  tone(BUZZER_PIN, 600, 200);
  delay(250);
  tone(BUZZER_PIN, 450, 200);
  delay(250);
  tone(BUZZER_PIN, 300, 400);
  delay(500);
  noTone(BUZZER_PIN);

  Serial.println("=========== GAME OVER ===========");
  Serial.print("Final score: ");
  Serial.print(score);
  Serial.print("   Level reached: ");
  Serial.println(level);
  Serial.println("Restarting in 3 seconds...");
  delay(3000);

  // Reset everything for a new game
  score = 0;
  level = 1;
  misses = 0;
  Serial.println("=========== NEW GAME ===========");
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));  // seed from noise on A0 so targets differ every run

  for (byte i = 0; i < TARGET_COUNT; i++) {
    pinMode(LED_PIN[i], OUTPUT);
    digitalWrite(LED_PIN[i], LOW);
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
    buttonRaw[i] = digitalRead(BUTTON_PIN[i]);
    buttonStable[i] = buttonRaw[i];
    buttonLastChange[i] = millis();
  }

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.println("=== Whack-a-Button Game ===");
  Serial.println("Hit the lit LED's button before the window closes.");
  Serial.println("You get " + String(MISSES_TO_END) + " misses. Speed rises every " +
                 String(HITS_PER_LEVEL) + " hits.");

  spawnTarget(millis());
  printStatus();
}

void loop() {
  unsigned long now = millis();

  int pressed = readButtonPress(now);

  if (targetActive) {
    // --- Did the player press the matching button? ---
    if (pressed >= 0 && pressed == activeTarget) {
      score++;
      tone(BUZZER_PIN, HIT_TONE_HZ, 60);        // short happy beep
      targetActive = false;
      clearTargets();

      int newLevel = (score / HITS_PER_LEVEL) + 1;  // level up every 5 hits
      if (newLevel != level) {
        level = newLevel;
        tone(BUZZER_PIN, HIT_TONE_HZ * 2, 120);  // extra beep on a level change
        Serial.print("LEVEL UP! Now level ");
        Serial.println(level);
      }

      printStatus();
      delay(150);            // tiny pause so one press cannot count twice
      spawnTarget(millis()); // next target right away
      return;
    }

    // Wrong button pressed = a miss
    if (pressed >= 0 && pressed != activeTarget) {
      misses++;
      tone(BUZZER_PIN, MISS_TONE_HZ, 200);  // low buzz for a mistake
      Serial.println("Wrong button - miss!");
      if (misses >= MISSES_TO_END) {
        gameOver();
        spawnTarget(millis());
        printStatus();
        return;
      }
      printStatus();
      delay(150);
      spawnTarget(millis());
      return;
    }

    // --- Time ran out for this target ---
    if (now - targetStartAt >= targetWindowMs) {
      misses++;
      tone(BUZZER_PIN, MISS_TONE_HZ, 200);
      Serial.println("Too slow - miss!");
      targetActive = false;
      clearTargets();

      if (misses >= MISSES_TO_END) {
        gameOver();
        spawnTarget(millis());
        printStatus();
        return;
      }

      printStatus();
      spawnTarget(millis());
      return;
    }
  } else {
    // Safety net: always have a target on screen
    spawnTarget(now);
  }

  // loop() repeats constantly, which is how the time window is measured
}
