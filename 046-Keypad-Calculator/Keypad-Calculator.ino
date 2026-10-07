// 046 - Keypad Calculator
// Type a two-operand expression such as "12+34" on a 4x4 membrane keypad and
// press # to see the answer on a 16x2 I2C LCD.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - Keypad             (by Mark Stanley and Alexander Brevig)
//   - LiquidCrystal I2C  (by Frank de Brabander)
//
// Wiring:
//   4x4 keypad ROW1 -> pin 9, ROW2 -> pin 8, ROW3 -> pin 7, ROW4 -> pin 6
//   4x4 keypad COL1 -> pin 5, COL2 -> pin 4, COL3 -> pin 3, COL4 -> pin 2
//   LCD 1602 (I2C 0x27) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND
//
// Key mapping (a 4x4 keypad has no maths symbols, so letters are reused):
//   1..9, 0  = digits
//   A = +      B = -      C = *      D = /
//   # = equals (evaluate)          * = clear

#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// ---------------------------------------------------------------------------
// Keypad wiring constants
// ---------------------------------------------------------------------------
const byte ROW_COUNT = 4;
const byte COL_COUNT = 4;

// NOTE: the Keypad library takes plain byte arrays (not const), so these two
// arrays must stay non-const.
byte ROW_PINS[ROW_COUNT] = {9, 8, 7, 6};   // row wires of the keypad
byte COL_PINS[COL_COUNT] = {5, 4, 3, 2};   // column wires of the keypad

// Physical layout of the keypad, read row by row
char KEY_MAP[ROW_COUNT][COL_COUNT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ---------------------------------------------------------------------------
// LCD constants
// ---------------------------------------------------------------------------
const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

const uint8_t MAX_ENTRY = 15;      // never type more than 15 characters
const uint8_t MAX_OPERAND_DIGITS = 4;  // each number may have up to 4 digits

// ---------------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------------
Keypad keypad = Keypad(makeKeymap(KEY_MAP), ROW_PINS, COL_PINS, ROW_COUNT, COL_COUNT);
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

char entry[MAX_ENTRY + 1];   // the expression typed so far, e.g. "12+34"
uint8_t entryLength = 0;
bool justEvaluated = false;  // true right after '=' was pressed
char resultLine[17];         // text shown on the second LCD line

// ---------------------------------------------------------------------------
// Translate a physical key into the character used by the calculator
// ---------------------------------------------------------------------------
char translateKey(char key) {
  switch (key) {
    case 'A': return '+';   // top row, rightmost key
    case 'B': return '-';
    case 'C': return '*';
    case 'D': return '/';   // bottom row, rightmost key
    case '#': return '=';   // enter / evaluate
    case '*': return 'C';   // clear
    default:  return key;   // digits 0..9 are used as they are
  }
}

// ---------------------------------------------------------------------------
// Clear the entry and the result display
// ---------------------------------------------------------------------------
void clearEntry() {
  entryLength = 0;
  entry[0] = '\0';
  justEvaluated = false;
  resultLine[0] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(F("                "));   // 16 spaces wipe the line
  lcd.setCursor(0, 1);
  lcd.print(F("Type 12+34 then #"));
  Serial.println(F("Entry cleared"));
}

// ---------------------------------------------------------------------------
// Validate and evaluate the stored expression.
// Returns true when the maths is valid; otherwise fills in the error message.
// ---------------------------------------------------------------------------
bool evaluateExpression(long &result, const char* &error) {
  error = nullptr;
  result = 0;

  if (entryLength == 0) {
    error = "EMPTY INPUT";
    return false;
  }

  // --- Find the operator and make sure there is exactly one -------------
  int opPos = -1;
  for (int i = 0; i < entryLength; i++) {
    if (entry[i] == '+' || entry[i] == '-' || entry[i] == '*' || entry[i] == '/') {
      opPos = i;
      break;
    }
  }
  if (opPos < 0)         { error = "NO OPERATOR";  return false; }
  if (opPos == 0)        { error = "MISSING LEFT"; return false; }
  if (opPos == entryLength - 1) { error = "MISSING RIGHT"; return false; }

  for (int i = opPos + 1; i < entryLength; i++) {
    if (strchr("+-*/", entry[i]) != nullptr) {
      error = "TWO OPERATORS";
      return false;
    }
  }

  // --- Validate both sides: digits only, not too long -------------------
  int leftLength = opPos;
  int rightLength = entryLength - opPos - 1;
  if (leftLength > MAX_OPERAND_DIGITS || rightLength > MAX_OPERAND_DIGITS) {
    error = "MAX 4 DIGITS";
    return false;
  }
  for (int i = 0; i < entryLength; i++) {
    if (i == opPos) continue;
    if (entry[i] < '0' || entry[i] > '9') {
      error = "BAD CHARACTER";
      return false;
    }
  }

  // --- Convert the two strings into numbers -----------------------------
  long a = 0, b = 0;
  for (int i = 0; i < leftLength;  i++) a = a * 10 + (entry[i] - '0');
  for (int i = opPos + 1; i < entryLength; i++) b = b * 10 + (entry[i] - '0');

  // --- Apply the operator ----------------------------------------------
  switch (entry[opPos]) {
    case '+': result = a + b; break;
    case '-': result = a - b; break;
    case '*': result = a * b; break;   // 4 digits x 4 digits still fits in a long
    case '/':
      if (b == 0) { error = "DIV BY ZERO"; return false; }
      result = a / b;                  // integer division, remainder dropped
      break;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Redraw both LCD lines from the current state
// ---------------------------------------------------------------------------
void refreshDisplay() {
  lcd.setCursor(0, 0);
  lcd.print(entry);
  for (uint8_t i = entryLength; i < LCD_COLUMNS; i++) lcd.print(' ');

  lcd.setCursor(0, 1);
  lcd.print(resultLine);
  for (uint8_t i = strlen(resultLine); i < LCD_COLUMNS; i++) lcd.print(' ');
}

// ---------------------------------------------------------------------------
// Handle one pressed key
// ---------------------------------------------------------------------------
void handleKey(char rawKey) {
  char key = translateKey(rawKey);

  // The physical '*' key clears everything
  if (key == 'C') {
    clearEntry();
    return;
  }

  // '=' evaluates the expression
  if (key == '=') {
    long result;
    const char* error;
    if (evaluateExpression(result, error)) {
      snprintf(resultLine, sizeof(resultLine), "= %ld", result);
      Serial.print(F("Expression: "));
      Serial.print(entry);
      Serial.print(F("  Result: "));
      Serial.println(result);
    } else {
      // '!' marks the line as an error; all messages are short enough for 16 columns
      snprintf(resultLine, sizeof(resultLine), "!%s", error);
      Serial.print(F("Validation error: "));
      Serial.println(error);
    }
    justEvaluated = true;
    refreshDisplay();
    return;
  }

  // A new digit starts a fresh expression after a calculation
  if (justEvaluated) {
    if (key >= '0' && key <= '9') {
      entryLength = 0;
      entry[0] = '\0';
      resultLine[0] = '\0';
    }
    justEvaluated = false;
  }

  // --- Validation while typing ------------------------------------------
  if (key >= '0' && key <= '9') {
    if (entryLength >= MAX_ENTRY) return;              // buffer full
    entry[entryLength++] = key;
    entry[entryLength] = '\0';
  } else {                                              // an operator
    if (entryLength == 0) return;                       // nothing to apply it to
    char last = entry[entryLength - 1];
    if (last == '+' || last == '-' || last == '*' || last == '/') {
      return;                                           // no two operators in a row
    }
    if (entryLength >= MAX_ENTRY) return;
    entry[entryLength++] = key;
    entry[entryLength] = '\0';
  }

  refreshDisplay();
  Serial.print(F("Typed: "));
  Serial.println(entry);
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("046 - Keypad Calculator starting..."));
  Serial.println(F("Keys: 0-9 digits, A=+ B=- C=* D=/, # equals, * clear"));

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Keypad Calculator"));
  lcd.setCursor(0, 1);
  lcd.print(F("Type 12+34 then #"));
  delay(1500);

  entry[0] = '\0';
  resultLine[0] = '\0';
  refreshDisplay();
}

void loop() {
  // The Keypad library scans all rows/columns and returns one key at a time
  char key = keypad.getKey();
  if (key != NO_KEY) {
    handleKey(key);
  }
}
