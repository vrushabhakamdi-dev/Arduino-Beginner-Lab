// 052 - Electronic Number Guessing Game
// The Arduino secretly picks a number between 1 and 100. You type guesses into
// the Serial Monitor and it answers HIGHER, LOWER or GUESSED, counts your
// attempts and plays a little celebration on the LED and buzzer when you win.
// Board: Arduino Uno
//
// Wiring:
//   Built-in LED -> pin 13 (already on the board, no extra wiring)
//   Passive buzzer (+) -> pin 8, buzzer (-) -> GND
//
// How to play: open the Serial Monitor at 9600 baud, type a number and press
// Enter. Send the letter r to restart with a new secret number.

// ------------------------------ Pin constants ------------------------------
const byte LED_PIN = 13;        // built-in LED shows the win celebration
const byte BUZZER_PIN = 8;      // passive buzzer plays the win tune

// ------------------------------ Game constants -----------------------------
const int MIN_NUMBER = 1;
const int MAX_NUMBER = 100;
const int WIN_TONE_HZ = 1000;   // pitch used while celebrating

// ------------------------------ Game state ---------------------------------
int secretNumber = 0;           // the number the player has to find
int attempts = 0;               // how many guesses were used in this game
bool gameOver = false;          // true once the number has been found

void newGame() {
  // random() gives a value from MIN_NUMBER up to (but not including) MAX_NUMBER + 1
  secretNumber = random(MIN_NUMBER, MAX_NUMBER + 1);
  attempts = 0;
  gameOver = false;
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.println();
  Serial.println("--- New game started ---");
  Serial.println("I picked a number from 1 to 100. Start guessing!");
}

// Every line typed in the Serial Monitor must be a plain whole number
bool isWholeNumber(const String &text) {
  if (text.length() == 0) return false;
  for (unsigned int i = 0; i < text.length(); i++) {
    if (!isDigit(text.charAt(i))) return false;  // any letter or symbol makes it invalid
  }
  return true;
}

// LED blinks three times and the buzzer plays a rising celebration
void celebrate() {
  Serial.print("GUESSED! The number was ");
  Serial.print(secretNumber);
  Serial.print(" and you needed ");
  Serial.print(attempts);
  Serial.println(" attempts.");

  for (byte i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, WIN_TONE_HZ + (i * 250), 150);  // rising pitch each flash
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  tone(BUZZER_PIN, 1800, 400);  // final higher beep

  Serial.println("Send r to play again.");
}

void handleGuess(int guess) {
  attempts++;

  if (guess < secretNumber) {
    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.println(" -> HIGHER");
  } else if (guess > secretNumber) {
    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.println(" -> LOWER");
  } else {
    gameOver = true;
    celebrate();
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);

  // Seed the random generator with electrical noise on A0 so every run
  // picks a different secret number (A0 is left unconnected on purpose)
  randomSeed(analogRead(A0));

  Serial.println("=== Electronic Number Guessing Game ===");
  Serial.println("Type a guess from 1 to 100 and press Enter.");
  Serial.println("Send r to restart at any time.");

  newGame();
}

void loop() {
  // Read one complete line from the Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // removes spaces, and the \r left by some terminals

    if (input.length() == 0) return;

    if (input.equalsIgnoreCase("r")) {  // restart command
      newGame();
      return;
    }

    if (!isWholeNumber(input)) {
      Serial.println("Please type a whole number from 1 to 100 (or r to restart).");
      return;
    }

    int guess = input.toInt();
    if (guess < MIN_NUMBER || guess > MAX_NUMBER) {
      Serial.println("That number is outside the range 1 to 100.");
      return;
    }

    if (gameOver) {
      Serial.println("This round is finished - send r to start a new game.");
      return;
    }

    handleGuess(guess);
  }

  // loop() keeps running so the Serial port is checked many times per second
}
