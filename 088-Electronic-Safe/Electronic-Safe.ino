// 088 - Electronic Safe
// A four digit keypad lock for a model safe: enter the PIN on a 4x4 keypad,
// the servo door swings open for 5 seconds, and three wrong attempts set off
// a 30 second lockout alarm. The PIN is kept in EEPROM so it survives a
// power cut and can be changed from the Serial Monitor.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - Keypad             (by Mark Stanley and Alexander Brevig)
//   - LiquidCrystal I2C  (by Frank de Brabander)
//   - Servo, EEPROM      (built into the Arduino IDE, nothing to install)
//
// Wiring:
//   4x4 keypad ROW1 -> pin 9, ROW2 -> pin 8, ROW3 -> pin 7, ROW4 -> pin 6
//   4x4 keypad COL1 -> pin 5, COL2 -> pin 4, COL3 -> pin 3, COL4 -> pin 2
//   Safe door servo signal -> pin 10 (0 deg = closed, 90 deg = open)
//   Piezo buzzer (+) -> pin 8, buzzer (-) -> GND
//   LCD 1602 (I2C address 0x27) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND
//
// NOTE: the buzzer sits on pin 8, which is also keypad row 2. The two never
// run at the same time because every beep is played while the keypad is not
// being scanned, so a key read and a beep cannot interfere with each other.
//
// Keys: 0..9 enter the PIN, # = confirm, * = clear
//
// CHANGING THE PIN: open the Serial Monitor at 9600 baud and send
//   N5678
// that is the letter N followed by exactly four digits. "No line ending" or
// "Newline" both work. The new PIN is written to EEPROM straight away.
//
// EEPROM layout: address 0 = magic byte 0xA5, addresses 1..4 = the four
// digits. On the very first boot the sketch writes the default PIN 1234.

#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

// ---------------------------------------------------------------------------
// Keypad hardware
// ---------------------------------------------------------------------------
const byte ROW_COUNT = 4;
const byte COL_COUNT = 4;
// NOTE: the Keypad library needs plain byte arrays (not const).
byte ROW_PINS[ROW_COUNT] = {9, 8, 7, 6};
byte COL_PINS[COL_COUNT] = {5, 4, 3, 2};

char KEY_MAP[ROW_COUNT][COL_COUNT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ---------------------------------------------------------------------------
// Pin and behaviour constants
// ---------------------------------------------------------------------------
const int SERVO_PIN = 10;        // safe door servo
const int BUZZER_PIN = 8;        // piezo buzzer (shares pin 8 with keypad row 2)

const int DOOR_CLOSED_ANGLE = 0;      // door fully shut
const int DOOR_OPEN_ANGLE = 90;       // door swung open
const unsigned long DOOR_OPEN_MS = 5000;  // door stays open for 5 seconds
const unsigned long LOCK_MESSAGE_MS = 600;  // how long "Safe LOCKED" stays up

const byte PIN_LENGTH = 4;            // PIN is four digits long
const byte MAX_TRIES = 3;             // wrong attempts before the lockout
const unsigned long LOCKOUT_MS = 30000UL;   // 30 second lockout
const unsigned long LOCKOUT_BEEP_MS = 500;  // alarm beep repetition

const byte EEPROM_MAGIC_ADDR = 0;
const byte EEPROM_PIN_ADDR = 1;
const byte EEPROM_MAGIC = 0xA5;
const char DEFAULT_PIN[] = "1234";

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------
Keypad keypad = Keypad(makeKeymap(KEY_MAP), ROW_PINS, COL_PINS, ROW_COUNT, COL_COUNT);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
Servo safeDoor;

char storedPin[PIN_LENGTH + 1];   // PIN read from EEPROM
char entered[PIN_LENGTH + 1];     // what the user is typing now
byte enteredLength = 0;

byte wrongTries = 0;                // consecutive wrong attempts
bool lockedOut = false;             // true during the 30 s lockout
bool doorOpen = false;              // true while the safe door is open
unsigned long lockoutStartMs = 0;
unsigned long doorOpenStartMs = 0;
unsigned long lastLockoutBeepMs = 0;
int lastCountdownSec = -1;

char serialBuf[PIN_LENGTH + 2];     // "N" + four digits + terminator
byte serialLen = 0;

// ---------------------------------------------------------------------------
// EEPROM helpers: the PIN survives a power cut
// ---------------------------------------------------------------------------
bool isDigits(const char* text) {
  for (byte i = 0; i < PIN_LENGTH; i++) {
    if (text[i] < '0' || text[i] > '9') return false;
  }
  return true;
}

void savePinToEeprom(const char* newPin) {
  EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  for (byte i = 0; i < PIN_LENGTH; i++) {
    EEPROM.update(EEPROM_PIN_ADDR + i, newPin[i]);  // update() only writes changes
  }
}

void loadPinFromEeprom() {
  // First boot (or cleared memory) -> store the default PIN
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
    savePinToEeprom(DEFAULT_PIN);
  }

  for (byte i = 0; i < PIN_LENGTH; i++) {
    storedPin[i] = (char)EEPROM.read(EEPROM_PIN_ADDR + i);
  }
  storedPin[PIN_LENGTH] = '\0';

  // Safety net: if the stored bytes are not four digits, fall back
  if (!isDigits(storedPin)) {
    savePinToEeprom(DEFAULT_PIN);
    strcpy(storedPin, DEFAULT_PIN);
  }
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------
void printLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  for (uint8_t i = strlen(text); i < LCD_COLUMNS; i++) lcd.print(' ');
}

void beep(unsigned long duration, unsigned int frequency) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);              // the keypad is not scanned during a beep
  noTone(BUZZER_PIN);
}

void showPrompt() {
  printLine(0, "Enter PIN:");
  char mask[PIN_LENGTH + 2];
  if (enteredLength == 0) {
    strcpy(mask, "_");
  } else {
    for (byte i = 0; i < enteredLength; i++) mask[i] = '*';
    mask[enteredLength] = '\0';
  }
  printLine(1, mask);
}

void clearEntry() {
  enteredLength = 0;
  entered[0] = '\0';
  showPrompt();
}

// ---------------------------------------------------------------------------
// Door control
// ---------------------------------------------------------------------------
void openDoor() {
  safeDoor.write(DOOR_OPEN_ANGLE);
  doorOpen = true;
  doorOpenStartMs = millis();
  Serial.println(F("Safe door: OPEN (closes after 5 s)"));
}

void closeDoor() {
  safeDoor.write(DOOR_CLOSED_ANGLE);
  doorOpen = false;
  Serial.println(F("Safe door: CLOSED"));
}

// ---------------------------------------------------------------------------
// PIN checking
// ---------------------------------------------------------------------------
void checkPin() {
  if (strcmp(entered, storedPin) == 0) {
    // --- correct PIN -----------------------------------------------------
    wrongTries = 0;
    printLine(0, "ACCESS GRANTED");
    printLine(1, "Door OPEN 5 s");
    Serial.println(F("ACCESS GRANTED"));
    beep(120, 1500);
    beep(120, 2000);
    openDoor();

    // Clear the buffer but keep the message on screen until the door closes
    enteredLength = 0;
    entered[0] = '\0';
    return;
  }

  // --- wrong PIN ---------------------------------------------------------
  wrongTries++;
  Serial.print(F("Wrong PIN, try "));
  Serial.print(wrongTries);
  Serial.print('/');
  Serial.println(MAX_TRIES);

  if (wrongTries >= MAX_TRIES) {
    // Three strikes -> lock the keypad out and sound the alarm
    lockedOut = true;
    lockoutStartMs = millis();
    lastLockoutBeepMs = 0;
    lastCountdownSec = -1;
    printLine(0, "TOO MANY TRIES");
    printLine(1, "LOCKED 30 s");
    Serial.println(F("LOCKED OUT for 30 seconds"));
    beep(800, 200);
    clearEntry();
    return;
  }

  char msg[17];
  snprintf(msg, sizeof(msg), "WRONG PIN %d/%d", wrongTries, MAX_TRIES);
  printLine(0, msg);
  printLine(1, "Try again");
  beep(300, 300);
  delay(900);
  clearEntry();
}

// ---------------------------------------------------------------------------
// Keypad handling (keys are ignored during the lockout and while open)
// ---------------------------------------------------------------------------
void handleKey(char key) {
  if (lockedOut) return;                 // keypad is dead for 30 seconds
  if (doorOpen) return;                  // let the door close on its own

  if (key == '*') {                      // clear the entry
    Serial.println(F("Entry cleared"));
    clearEntry();
    beep(80, 1000);
    return;
  }

  if (key == '#') {                      // confirm the entry
    if (enteredLength == PIN_LENGTH) {
      beep(80, 1000);
      checkPin();
    } else {
      printLine(0, "ENTER 4 DIGITS");
      beep(80, 1000);
      delay(800);
      showPrompt();
    }
    return;
  }

  if (key < '0' || key > '9') return;    // A..D are not used

  if (enteredLength < PIN_LENGTH) {
    entered[enteredLength++] = key;
    entered[enteredLength] = '\0';
    showPrompt();
    beep(50, 1200);
    Serial.print(F("Digit entered, length = "));
    Serial.println(enteredLength);
  }
}

// ---------------------------------------------------------------------------
// Changing the PIN over Serial, expected format: N1234
// ---------------------------------------------------------------------------
void applySerialPin(const char* buffer) {
  if (buffer[0] != 'N' || !isDigits(buffer + 1)) {
    Serial.println(F("Invalid. Send N followed by 4 digits, e.g. N5678"));
    return;
  }

  char newPin[PIN_LENGTH + 1];
  memcpy(newPin, buffer + 1, PIN_LENGTH);
  newPin[PIN_LENGTH] = '\0';

  savePinToEeprom(newPin);
  strcpy(storedPin, newPin);

  Serial.print(F("PIN changed to: "));
  Serial.println(storedPin);
  printLine(0, "PIN CHANGED");
  printLine(1, storedPin);
  delay(1000);
  showPrompt();
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialLen == 0) continue;      // swallow the second of CR/LF
      serialBuf[serialLen] = '\0';
      serialLen = 0;
      applySerialPin(serialBuf);
      continue;
    }

    if (serialLen < PIN_LENGTH + 1) {
      serialBuf[serialLen++] = c;

      // Five characters collected -> act even if no newline was sent
      if (serialLen == PIN_LENGTH + 1) {
        serialBuf[serialLen] = '\0';
        serialLen = 0;
        applySerialPin(serialBuf);
      }
    } else {
      serialLen = 0;                     // overlong input, start again
    }
  }
}

// ---------------------------------------------------------------------------
// Lockout countdown with a repeating alarm beep
// ---------------------------------------------------------------------------
void updateLockout() {
  if (!lockedOut) return;

  unsigned long elapsed = millis() - lockoutStartMs;

  if (elapsed >= LOCKOUT_MS) {
    lockedOut = false;
    wrongTries = 0;
    noTone(BUZZER_PIN);
    Serial.println(F("Lockout finished, keypad unlocked"));
    beep(120, 1500);
    clearEntry();
    return;
  }

  // Alarm beep every 500 ms while locked out
  if (millis() - lastLockoutBeepMs >= LOCKOUT_BEEP_MS) {
    lastLockoutBeepMs = millis();
    tone(BUZZER_PIN, 700, 80);
  }

  // Refresh the countdown on the LCD once per second
  int remaining = (int)((LOCKOUT_MS - elapsed + 999) / 1000);
  if (remaining != lastCountdownSec) {
    lastCountdownSec = remaining;
    char msg[17];
    snprintf(msg, sizeof(msg), "LOCKED %d s", remaining);
    printLine(1, msg);
    Serial.println(msg);
  }
}

// ---------------------------------------------------------------------------
// Close the door again after its 5 second window
// ---------------------------------------------------------------------------
void updateDoor() {
  if (!doorOpen) return;
  if (millis() - doorOpenStartMs < DOOR_OPEN_MS) return;

  closeDoor();
  printLine(0, "Safe LOCKED");
  delay(LOCK_MESSAGE_MS);
  clearEntry();                 // back to a fresh prompt
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("088 - Electronic Safe starting..."));
  Serial.println(F("Send N and four digits (example: N5678) to change the PIN."));

  pinMode(BUZZER_PIN, OUTPUT);

  safeDoor.attach(SERVO_PIN);
  safeDoor.write(DOOR_CLOSED_ANGLE);   // always boot with the safe shut

  lcd.init();
  lcd.backlight();
  lcd.clear();
  printLine(0, "Electronic Safe");
  printLine(1, "Loading PIN...");
  delay(1200);

  loadPinFromEeprom();
  Serial.print(F("Current PIN: "));
  Serial.println(storedPin);

  clearEntry();
}

void loop() {
  updateLockout();              // keeps the 30 s alarm running
  updateDoor();                 // closes the door after 5 seconds
  handleSerial();               // accepts a new PIN from the Serial Monitor

  char key = keypad.getKey();   // scan the keypad
  if (key != NO_KEY) {
    handleKey(key);
  }

  delay(10);
}
