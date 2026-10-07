// 051 - Push Button Quiz Buzzer
// A three-player classroom quiz buzzer: the first player to press their button
// wins the round, the other two players are locked out, the winner LED lights
// up and a beep sounds. The host presses a reset button to open the next round.
// Board: Arduino Uno
//
// Wiring:
//   Player 1 button -> pin 2, Player 2 button -> pin 3, Player 3 button -> pin 4
//   Host reset button -> pin 5
//     (each button connects its pin to GND; the internal pull-up is enabled)
//   Player 1 LED -> pin 6, Player 2 LED -> pin 7, Player 3 LED -> pin 8
//     (LED anode -> pin through a 220 ohm resistor, LED cathode -> GND)
//   Passive buzzer (+) -> pin 9, buzzer (-) -> GND

// ------------------------------ Pin constants ------------------------------
const byte PLAYER_COUNT = 3;
const byte PLAYER_BUTTON_PIN[PLAYER_COUNT] = {2, 3, 4};  // one buzz-in button per player
const byte PLAYER_LED_PIN[PLAYER_COUNT] = {6, 7, 8};     // one winner LED per player
const byte HOST_BUTTON_PIN = 5;                          // host resets for the next question
const byte BUZZER_PIN = 9;                               // winner beep

// ------------------------------ Timing constants ---------------------------
const unsigned long DEBOUNCE_MS = 30;  // ignore contact chatter shorter than 30 ms
const int WIN_TONE_HZ = 1500;          // pitch of the victory beep
const unsigned long WIN_TONE_MS = 500; // length of the victory beep

// ------------------------------ Button debounce state ----------------------
bool playerRaw[PLAYER_COUNT];               // raw reading, changes on every contact bounce
bool playerStable[PLAYER_COUNT];            // settled reading after debouncing
unsigned long playerLastChange[PLAYER_COUNT];
bool hostRaw;
bool hostStable;
unsigned long hostLastChange;

// ------------------------------ Game state ---------------------------------
bool roundActive = true;  // true while the round still waits for a buzz-in
int winnerIndex = -1;     // -1 means nobody has buzzed in yet

// Reads one button and returns true only ONCE, at the moment it is pressed.
// The 30 ms debounce window stops a single physical press from counting twice.
bool detectPress(byte pin, bool &raw, bool &stable, unsigned long &lastChange, unsigned long now) {
  bool reading = digitalRead(pin);  // HIGH = released, LOW = pressed (active LOW)

  if (reading != raw) {        // reading changed, restart the debounce timer
    raw = reading;
    lastChange = now;
  }

  if ((now - lastChange) >= DEBOUNCE_MS && reading != stable) {
    stable = reading;
    if (stable == LOW) return true;  // press edge detected
  }
  return false;
}

void startRound() {
  roundActive = true;
  winnerIndex = -1;
  noTone(BUZZER_PIN);
  for (byte i = 0; i < PLAYER_COUNT; i++) {
    digitalWrite(PLAYER_LED_PIN[i], LOW);  // clear the old winner LED
  }
  Serial.println("--- New round: waiting for the first buzz-in ---");
}

void registerWinner(byte index) {
  roundActive = false;  // lock out every other player from now on
  winnerIndex = index;

  // Light only the winner LED
  for (byte i = 0; i < PLAYER_COUNT; i++) {
    digitalWrite(PLAYER_LED_PIN[i], (i == index) ? HIGH : LOW);
  }

  tone(BUZZER_PIN, WIN_TONE_HZ, WIN_TONE_MS);  // short victory beep

  Serial.print("WINNER: Player ");
  Serial.print(index + 1);
  Serial.println(" buzzed in first - round is now locked.");
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < PLAYER_COUNT; i++) {
    pinMode(PLAYER_BUTTON_PIN[i], INPUT_PULLUP);  // pull-up so the button only needs to touch GND
    pinMode(PLAYER_LED_PIN[i], OUTPUT);
    digitalWrite(PLAYER_LED_PIN[i], LOW);
    playerRaw[i] = digitalRead(PLAYER_BUTTON_PIN[i]);
    playerStable[i] = playerRaw[i];
    playerLastChange[i] = millis();
  }

  pinMode(HOST_BUTTON_PIN, INPUT_PULLUP);
  hostRaw = digitalRead(HOST_BUTTON_PIN);
  hostStable = hostRaw;
  hostLastChange = millis();

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Serial.println("Push Button Quiz Buzzer ready.");
  Serial.println("First player to press buzzes in and wins the round.");
  Serial.println("Host: press the button on pin 5 to reset for the next question.");
}

void loop() {
  unsigned long now = millis();

  // --- Host reset button: always available, opens a fresh round ---
  if (detectPress(HOST_BUTTON_PIN, hostRaw, hostStable, hostLastChange, now)) {
    startRound();
  }

  // --- Player buzz-in buttons ---
  for (byte i = 0; i < PLAYER_COUNT; i++) {
    bool pressed = detectPress(PLAYER_BUTTON_PIN[i], playerRaw[i],
                               playerStable[i], playerLastChange[i], now);

    if (!pressed) continue;

    if (roundActive) {
      registerWinner(i);   // first press wins and locks the round
      break;               // winner found, no need to check the other players
    } else {
      // Priority logic: the round is already locked, so this press is ignored
      Serial.print("Press from Player ");
      Serial.print(i + 1);
      Serial.println(" ignored - someone already buzzed in.");
    }
  }

  // loop() repeats continuously, so every button is scanned many times per second
}
