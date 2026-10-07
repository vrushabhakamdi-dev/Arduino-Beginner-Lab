// 047 - Keypad Digital Lock
// A 4-digit password lock: type the PIN on a 4x4 keypad, press # to open.
// Three wrong attempts lock the keypad out for 30 seconds.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - Keypad             (by Mark Stanley and Alexander Brevig)
//   - LiquidCrystal I2C  (by Frank de Brabander)
//
// Wiring:
//   4x4 keypad ROW1 -> pin 9, ROW2 -> pin 8, ROW3 -> pin 7, ROW4 -> pin 6
//   4x4 keypad COL1 -> pin 5, COL2 -> pin 4, COL3 -> pin 3, COL4 -> pin 2
//   Piezo buzzer (+) -> pin 8, buzzer (-) -> GND
//   LCD 1602 (I2C 0x27) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND
//
// Keys: 0..9 enter the PIN, # = confirm, * = clear

#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// ---------------------------------------------------------------------------
// Constants (named constants instead of magic numbers)
// ---------------------------------------------------------------------------
const char PIN_CODE[] = "1234";        // <-- the password, change it here
const uint8_t PIN_LENGTH = 4;          // number of digits in the PIN
const uint8_t MAX_TRIES = 3;           // wrong attempts before the lockout
const unsigned long LOCKOUT_MS = 30000UL;  // 30 second lockout
const unsigned long BEEP_MS = 120;     // length of a key click

const uint8_t BUZZER_PIN = 8;          // piezo buzzer

const byte ROW_COUNT = 4;
const byte COL_COUNT = 4;
// NOTE: the Keypad library takes plain byte arrays (not const), so these two
// arrays must stay non-const.
byte ROW_PINS[ROW_COUNT] = {9, 8, 7, 6};   // keypad row wires
byte COL_PINS[COL_COUNT] = {5, 4, 3, 2};   // keypad column wires

char KEY_MAP[ROW_COUNT][COL_COUNT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------
Keypad keypad = Keypad(makeKeymap(KEY_MAP), ROW_PINS, COL_PINS, ROW_COUNT, COL_COUNT);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

char entered[PIN_LENGTH + 1];   // what the user has typed so far
uint8_t enteredLength = 0;

uint8_t wrongTries = 0;               // consecutive wrong attempts
bool lockedOut = false;               // true during the 30 second lockout
unsigned long lockoutStartMs = 0;     // when the lockout began

// ---------------------------------------------------------------------------
// Show a line of text on the LCD and wipe the rest of that line
// ---------------------------------------------------------------------------
void printLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  for (uint8_t i = strlen(text); i < LCD_COLUMNS; i++) lcd.print(' ');
}

// ---------------------------------------------------------------------------
// Buzzer helpers
// ---------------------------------------------------------------------------
void beep(unsigned long duration, unsigned int frequency) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);          // short blocking beep keeps the feedback obvious
  noTone(BUZZER_PIN);
}

void successBeep() {
  beep(120, 1500);          // two quick high beeps for "open"
  beep(120, 2000);
}

void errorBeep() {
  beep(300, 300);           // one low buzz for "wrong"
}

// ---------------------------------------------------------------------------
// Ready the display for a new PIN entry
// ---------------------------------------------------------------------------
void showPrompt() {
  printLine(0, "Enter PIN:");
  char mask[17];
  for (uint8_t i = 0; i < enteredLength; i++) mask[i] = '*';
  mask[enteredLength] = '\0';
  if (enteredLength == 0) strcpy(mask, "_");   // cursor style hint
  printLine(1, mask);
}

void clearEntry() {
  enteredLength = 0;
  entered[0] = '\0';
  showPrompt();
}

// ---------------------------------------------------------------------------
// Check the typed PIN against the stored code
// ---------------------------------------------------------------------------
void checkPin() {
  Serial.print(F("PIN entered: "));
  Serial.println(entered);

  if (strcmp(entered, PIN_CODE) == 0) {
    // --- correct PIN ----------------------------------------------------
    wrongTries = 0;
    lockedOut = false;
    printLine(0, "ACCESS GRANTED");
    printLine(1, "Door is OPEN");
    Serial.println(F("ACCESS GRANTED"));
    successBeep();
    delay(1500);            // keep the message visible for a moment
  } else {
    // --- wrong PIN ------------------------------------------------------
    wrongTries++;
    Serial.print(F("Wrong PIN, try "));
    Serial.print(wrongTries);
    Serial.print('/');
    Serial.println(MAX_TRIES);

    if (wrongTries >= MAX_TRIES) {
      // Three strikes -> lock out for 30 seconds
      lockedOut = true;
      lockoutStartMs = millis();
      printLine(0, "TOO MANY TRIES");
      printLine(1, "LOCKED 30 s");
      Serial.println(F("LOCKED OUT for 30 seconds"));
      beep(800, 200);       // long low warning sound
    } else {
      char msg[17];
      snprintf(msg, sizeof(msg), "WRONG PIN %d/%d", wrongTries, MAX_TRIES);
      printLine(0, msg);
      printLine(1, "Try again");
      errorBeep();
      delay(1000);
    }
  }
  clearEntry();
}

// ---------------------------------------------------------------------------
// Handle one key press (ignored while the lockout is running)
// ---------------------------------------------------------------------------
void handleKey(char key) {
  if (lockedOut) return;

  if (key == '*') {                 // clear
    Serial.println(F("Cleared"));
    clearEntry();
    beep(BEEP_MS, 1000);
    return;
  }

  if (key == '#') {                 // confirm
    if (enteredLength == PIN_LENGTH) {
      beep(BEEP_MS, 1000);
      checkPin();
    } else {
      printLine(0, "ENTER 4 DIGITS");
      beep(BEEP_MS, 1000);
      delay(800);
      showPrompt();
    }
    return;
  }

  // Digits A..D are simply ignored, only 0..9 are accepted
  if (key < '0' || key > '9') return;

  if (enteredLength < PIN_LENGTH) {
    entered[enteredLength++] = key;
    entered[enteredLength] = '\0';
    showPrompt();
    beep(BEEP_MS, 1200);            // soft click for every digit
    Serial.print(F("Digit pressed, length = "));
    Serial.println(enteredLength);
  }
}

// ---------------------------------------------------------------------------
// Count the lockout down and update the display once per second
// ---------------------------------------------------------------------------
void updateLockout() {
  if (!lockedOut) return;

  unsigned long elapsed = millis() - lockoutStartMs;
  if (elapsed >= LOCKOUT_MS) {
    // Lockout finished -> allow new attempts
    lockedOut = false;
    wrongTries = 0;
    Serial.println(F("Lockout finished, keypad unlocked"));
    successBeep();
    showPrompt();
    return;
  }

  // Remaining seconds, rounded up so it starts at 30
  unsigned long remaining = (LOCKOUT_MS - elapsed + 999) / 1000;
  static unsigned long lastShown = 999;
  if (remaining != lastShown) {
    char msg[17];
    snprintf(msg, sizeof(msg), "LOCKED %lu s", remaining);
    printLine(1, msg);
    lastShown = remaining;
    Serial.println(msg);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("047 - Keypad Digital Lock starting..."));
  Serial.print(F("PIN is: "));
  Serial.println(PIN_CODE);

  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  printLine(0, "Keypad Lock");
  printLine(1, "PIN length 4");
  delay(1500);

  clearEntry();
}

void loop() {
  updateLockout();                        // keep the lockout countdown running

  char key = keypad.getKey();             // scan the keypad
  if (key != NO_KEY) {
    handleKey(key);
  }
}
