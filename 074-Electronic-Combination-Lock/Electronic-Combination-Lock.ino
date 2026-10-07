// 074 - Electronic Combination Lock
// Enter a 4 digit PIN on a 4x4 membrane keypad to open a servo driven lock.
// The PIN can be changed with a Serial command and is stored in EEPROM,
// so it survives a power cycle. Three wrong attempts trigger a 30 second alarm.
// Board: Arduino Uno
// Wiring:
//   4x4 keypad rows -> pins 9, 8, 7, 6
//   4x4 keypad cols -> pins 5, 4, 3, 2
//   Servo signal -> pin 10 (0 deg = locked, 90 deg = open)
//   Buzzer -> pin 11 (other leg to GND)
//
// Libraries: Keypad (install from the Library Manager), EEPROM (built-in)

#include <Keypad.h>   // Library for reading a membrane keypad
#include <EEPROM.h>   // Built-in library to remember the PIN without power
#include <Servo.h>    // Built-in library for the lock servo

// ---------------- Keypad wiring ----------------
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};  // Rows connected to these pins
byte colPins[COLS] = {5, 4, 3, 2};  // Columns connected to these pins

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- Other pins ----------------
const int SERVO_PIN = 10;   // Servo that moves the lock bolt
const int BUZZER_PIN = 11;  // Buzzer for feedback and the alarm

// ---------------- Lock settings ----------------
const int LOCKED_ANGLE = 0;             // Servo angle when locked
const int OPEN_ANGLE = 90;              // Servo angle when unlocked
const unsigned long UNLOCK_TIME_MS = 5000;   // Auto relock after 5 seconds
const unsigned long ALARM_TIME_MS = 30000;   // Alarm lasts 30 seconds
const unsigned long ALARM_BLINK_MS = 250;    // Alarm toggles 4 times a second
const byte MAX_WRONG_ATTEMPTS = 3;           // Wrong PINs before the alarm
const byte PIN_LENGTH = 4;                   // PIN is exactly 4 digits

// ---------------- EEPROM layout ----------------
const int EE_ADDR_PIN = 0;      // 4 bytes, one per digit
const int EE_ADDR_MAGIC = 4;    // 1 byte marker: EEPROM already initialised?
const byte EE_MAGIC_VALUE = 0xA5;
const char DEFAULT_PIN[PIN_LENGTH + 1] = "2580";  // Factory PIN

// ---------------- State ----------------
enum LockState { LOCKED, OPENED, ALARMING };

LockState lockState = LOCKED;
char storedPin[PIN_LENGTH + 1] = "2580";  // Current PIN (kept in RAM too)
char entered[PIN_LENGTH + 1] = "";        // What the user has typed so far
byte enteredLength = 0;
byte wrongAttempts = 0;
unsigned long stateStartMs = 0;   // When the current state began
unsigned long lastAlarmToggleMs = 0;
bool alarmToneOn = false;

Servo lockServo;

// ---------------- EEPROM helpers ----------------
void savePinToEeprom(const char *newPin) {
  for (byte i = 0; i < PIN_LENGTH; i++) {
    // EEPROM.update() only writes when the value really changes,
    // which saves the limited number of EEPROM write cycles
    EEPROM.update(EE_ADDR_PIN + i, (byte)newPin[i]);
    storedPin[i] = newPin[i];
  }
  storedPin[PIN_LENGTH] = '\0';
  EEPROM.update(EE_ADDR_MAGIC, EE_MAGIC_VALUE);
}

// Read the stored PIN, or write the default one the very first time
void loadPinFromEeprom() {
  if (EEPROM.read(EE_ADDR_MAGIC) != EE_MAGIC_VALUE) {
    // First run: save the default PIN so the address holds something valid
    savePinToEeprom(DEFAULT_PIN);
    Serial.println("EEPROM was empty - default PIN 2580 stored.");
    return;
  }
  for (byte i = 0; i < PIN_LENGTH; i++) {
    storedPin[i] = (char)EEPROM.read(EE_ADDR_PIN + i);
  }
  storedPin[PIN_LENGTH] = '\0';
}

// ---------------- Feedback helpers ----------------
void beep(int frequency, int durationMs) {
  tone(BUZZER_PIN, frequency);
  delay(durationMs);
  noTone(BUZZER_PIN);
}

// Empty the entry buffer so no characters from an old attempt survive
void clearEntry() {
  enteredLength = 0;
  for (byte i = 0; i <= PIN_LENGTH; i++) entered[i] = '\0';
}

void lockAgain() {
  lockServo.write(LOCKED_ANGLE);
  lockState = LOCKED;
  clearEntry();
  Serial.println("LOCK is now LOCKED. Enter PIN.");
}

void unlock() {
  lockServo.write(OPEN_ANGLE);
  lockState = OPENED;
  stateStartMs = millis();
  wrongAttempts = 0;  // Successful entry clears the attempt counter
  clearEntry();
  beep(1500, 120);
  Serial.println("LOCK OPENED - door unlocked for 5 seconds.");
}

void startAlarm() {
  lockState = ALARMING;
  stateStartMs = millis();
  lastAlarmToggleMs = millis();
  alarmToneOn = true;
  tone(BUZZER_PIN, 2000);
  Serial.println("ALARM! 3 wrong attempts - sounding for 30 seconds.");
}

// Check what the user typed against the stored PIN
void checkEnteredPin() {
  entered[PIN_LENGTH] = '\0';
  Serial.print("PIN entered: ");
  for (byte i = 0; i < PIN_LENGTH; i++) Serial.print('*');  // Never print the real code
  Serial.println();

  // Only a complete 4 digit entry can ever be correct
  bool correct = (enteredLength == PIN_LENGTH);
  for (byte i = 0; correct && i < PIN_LENGTH; i++) {
    if (entered[i] != storedPin[i]) correct = false;
  }

  if (correct) {
    unlock();
  } else {
    wrongAttempts++;
    clearEntry();
    beep(300, 300);  // Low error tone
    Serial.print("WRONG PIN - attempt ");
    Serial.print(wrongAttempts);
    Serial.print(" of ");
    Serial.println(MAX_WRONG_ATTEMPTS);

    if (wrongAttempts >= MAX_WRONG_ATTEMPTS) {
      startAlarm();
    }
  }
}

// Handle one line received from the Serial Monitor, e.g. "PIN=1234"
void handleSerialCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  if (line.startsWith("PIN=")) {
    String candidate = line.substring(4);
    if (candidate.length() != PIN_LENGTH) {
      Serial.println("ERROR: the new PIN must be exactly 4 digits. Example: PIN=1379");
      return;
    }
    bool allDigits = true;
    for (byte i = 0; i < PIN_LENGTH; i++) {
      if (candidate.charAt(i) < '0' || candidate.charAt(i) > '9') allDigits = false;
    }
    if (!allDigits) {
      Serial.println("ERROR: use digits 0-9 only. Example: PIN=1379");
      return;
    }
    savePinToEeprom(candidate.c_str());
    Serial.println("PIN changed and saved to EEPROM. Use it on the keypad now.");
  } else {
    Serial.println("Available command: PIN=1234  (exactly 4 digits)");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Electronic Combination Lock starting...");

  lockServo.attach(SERVO_PIN);
  pinMode(BUZZER_PIN, OUTPUT);

  loadPinFromEeprom();

  lockServo.write(LOCKED_ANGLE);
  Serial.print("Stored PIN is ");
  Serial.print(PIN_LENGTH);
  Serial.println(" digits long.");
  Serial.println("Type a key on the keypad. '*' clears your entry.");
  Serial.println("Change the PIN from the Serial Monitor with: PIN=1379");
}

void loop() {
  // ----- 1. Always accept Serial commands (PIN changes) -----
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    handleSerialCommand(line);
  }

  // ----- 2. Read the keypad -----
  char key = keypad.getKey();
  if (key != NO_KEY && lockState == LOCKED) {
    if (key == '*') {
      // Clear the current entry
      clearEntry();
      Serial.println("Entry cleared.");
      beep(800, 60);
    } else if (key == '#') {
      // Optional: press '#' to submit (same result as typing the 4th digit)
      if (enteredLength == PIN_LENGTH) checkEnteredPin();
    } else if (key >= '0' && key <= '9') {
      if (enteredLength < PIN_LENGTH) {
        entered[enteredLength] = key;
        enteredLength++;
        beep(1000, 40);  // Short click for feedback
        Serial.print("Entered so far: ");
        for (byte i = 0; i < enteredLength; i++) Serial.print('*');
        Serial.println();
      }
      // Auto-submit as soon as 4 digits are in
      if (enteredLength == PIN_LENGTH) checkEnteredPin();
    }
  } else if (key != NO_KEY && lockState != LOCKED) {
    // Keys are ignored while the lock is open or the alarm is sounding
    beep(500, 60);
  }

  // ----- 3. Manage the OPENED state (auto relock) -----
  if (lockState == OPENED && millis() - stateStartMs >= UNLOCK_TIME_MS) {
    lockAgain();
  }

  // ----- 4. Manage the ALARMING state (30 seconds) -----
  if (lockState == ALARMING) {
    // Toggle the siren tone to make it hard to ignore
    if (millis() - lastAlarmToggleMs >= ALARM_BLINK_MS) {
      lastAlarmToggleMs = millis();
      alarmToneOn = !alarmToneOn;
      if (alarmToneOn) tone(BUZZER_PIN, 2000);
      else noTone(BUZZER_PIN);
    }

    if (millis() - stateStartMs >= ALARM_TIME_MS) {
      noTone(BUZZER_PIN);
      wrongAttempts = 0;
      Serial.println("Alarm finished - lock reset.");
      lockAgain();
    }
  }

  delay(10);
}
