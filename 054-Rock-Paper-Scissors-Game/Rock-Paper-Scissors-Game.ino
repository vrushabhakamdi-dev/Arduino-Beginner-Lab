// 054 - Rock Paper Scissors Game
// Play best-of-five Rock Paper Scissors against the Arduino. Press one of the
// three buttons to throw your move, watch the win / lose / draw LEDs and read
// the full battle log with running scores in the Serial Monitor.
// Board: Arduino Uno
//
// Wiring:
//   Rock button -> pin 2, Paper button -> pin 3, Scissors button -> pin 4
//     (each button shorts its pin to GND; internal pull-ups are enabled)
//   Win LED -> pin 5, Lose LED -> pin 6, Draw LED -> pin 7
//     (LED anode to the pin through a 220 ohm resistor, cathode to GND)
//   No buzzer is needed for this game.

// ------------------------------ Pin constants ------------------------------
const byte ROCK_PIN = 2;
const byte PAPER_PIN = 3;
const byte SCISSORS_PIN = 4;
const byte WIN_LED_PIN = 5;
const byte LOSE_LED_PIN = 6;
const byte DRAW_LED_PIN = 7;
const byte BUTTON_COUNT = 3;
const byte BUTTON_PIN[BUTTON_COUNT] = {ROCK_PIN, PAPER_PIN, SCISSORS_PIN};

// ------------------------------ Game constants -----------------------------
const int POINTS_TO_WIN = 3;      // best of five: first player to 3 takes the match
const unsigned long DEBOUNCE_MS = 30;
const unsigned long LED_SHOW_MS = 1500;  // how long the result LEDs stay on

// Move numbers match the button order: 0 = Rock, 1 = Paper, 2 = Scissors
const char *MOVE_NAME[3] = {"Rock", "Paper", "Scissors"};

// ------------------------------ Game state ---------------------------------
int playerScore = 0;
int cpuScore = 0;
int roundsPlayed = 0;

// Button debounce state
bool buttonRaw[BUTTON_COUNT];
bool buttonStable[BUTTON_COUNT];
unsigned long buttonLastChange[BUTTON_COUNT];

// Returns the index (0-2) of the button just pressed, or -1 for "nothing"
int readButtonPress(unsigned long now) {
  int result = -1;

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    bool reading = digitalRead(BUTTON_PIN[i]);  // LOW = pressed

    if (reading != buttonRaw[i]) {
      buttonRaw[i] = reading;
      buttonLastChange[i] = now;
    }

    if ((now - buttonLastChange[i]) >= DEBOUNCE_MS && reading != buttonStable[i]) {
      buttonStable[i] = reading;
      if (buttonStable[i] == LOW && result == -1) result = i;
    }
  }
  return result;
}

// Rock crushes Scissors, Scissors cuts Paper, Paper covers Rock
// Returns 1 for player win, -1 for player loss, 0 for a draw
int judge(int playerMove, int cpuMove) {
  if (playerMove == cpuMove) return 0;
  if ((playerMove == 0 && cpuMove == 2) ||
      (playerMove == 1 && cpuMove == 0) ||
      (playerMove == 2 && cpuMove == 1)) {
    return 1;
  }
  return -1;
}

void showResultLeds(int result) {
  digitalWrite(WIN_LED_PIN, (result == 1) ? HIGH : LOW);
  digitalWrite(LOSE_LED_PIN, (result == -1) ? HIGH : LOW);
  digitalWrite(DRAW_LED_PIN, (result == 0) ? HIGH : LOW);
}

void clearResultLeds() {
  digitalWrite(WIN_LED_PIN, LOW);
  digitalWrite(LOSE_LED_PIN, LOW);
  digitalWrite(DRAW_LED_PIN, LOW);
}

void playRound(int playerMove, int cpuMove) {
  int result = judge(playerMove, cpuMove);
  roundsPlayed++;

  if (result == 1) playerScore++;
  else if (result == -1) cpuScore++;

  // --- Battle log for the Serial Monitor ---
  Serial.print("Round ");
  Serial.print(roundsPlayed);
  Serial.print(":  You = ");
  Serial.print(MOVE_NAME[playerMove]);
  Serial.print("  |  CPU = ");
  Serial.print(MOVE_NAME[cpuMove]);
  Serial.print("  ->  ");
  if (result == 1) Serial.print("YOU WIN!  ");
  else if (result == -1) Serial.print("CPU WINS!  ");
  else Serial.print("DRAW!  ");
  Serial.print("(Score ");
  Serial.print(playerScore);
  Serial.print(" - ");
  Serial.print(cpuScore);
  Serial.println(")");

  showResultLeds(result);
  delay(LED_SHOW_MS);   // hold the result LEDs on so the player can read them
  clearResultLeds();

  // --- Match over? First to 3 points wins the best of five ---
  if (playerScore >= POINTS_TO_WIN || cpuScore >= POINTS_TO_WIN) {
    Serial.println("----------------------------------------");
    if (playerScore > cpuScore) Serial.println("MATCH RESULT: You beat the CPU!");
    else Serial.println("MATCH RESULT: The CPU wins the match.");
    Serial.print("Final score: ");
    Serial.print(playerScore);
    Serial.print(" - ");
    Serial.println(cpuScore);
    Serial.println("Starting a new match...");
    Serial.println("----------------------------------------");
    playerScore = 0;
    cpuScore = 0;
    roundsPlayed = 0;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(ROCK_PIN, INPUT_PULLUP);
  pinMode(PAPER_PIN, INPUT_PULLUP);
  pinMode(SCISSORS_PIN, INPUT_PULLUP);

  pinMode(WIN_LED_PIN, OUTPUT);
  pinMode(LOSE_LED_PIN, OUTPUT);
  pinMode(DRAW_LED_PIN, OUTPUT);
  clearResultLeds();

  // Give every button a known starting state
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    buttonRaw[i] = digitalRead(BUTTON_PIN[i]);
    buttonStable[i] = buttonRaw[i];
    buttonLastChange[i] = millis();
  }

  Serial.println("=== Rock Paper Scissors - best of five ===");
  Serial.println("Rock = pin 2 button, Paper = pin 3 button, Scissors = pin 4 button");
  Serial.println("First to 3 points wins the match.");
}

void loop() {
  int pressed = readButtonPress(millis());

  if (pressed >= 0) {
    // The CPU picks its move at random: 0 = Rock, 1 = Paper, 2 = Scissors
    int cpuMove = random(0, 3);
    playRound(pressed, cpuMove);
  }

  // loop() keeps scanning the buttons many times per second
}
