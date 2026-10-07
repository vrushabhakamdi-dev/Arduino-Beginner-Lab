// 045 - LCD Calculator
// A tiny calculator: two potentiometers act as the two numbers and four push
// buttons choose +, -, * or /. The sum is shown on a 16x2 I2C LCD like
// "12 + 34 = 46". Integer math only.
// Board: Arduino Uno
//
// Library: LiquidCrystal I2C (by Frank de Brabander)
//
// Wiring:
//   LCD 1602 (I2C 0x27) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND
//   Potentiometer A  -> wiper (middle leg) to A0, outer legs to 5V and GND
//   Potentiometer B  -> wiper (middle leg) to A1, outer legs to 5V and GND
//   Button "+" -> pin 2 to GND      Button "-" -> pin 3 to GND
//   Button "*" -> pin 4 to GND      Button "/" -> pin 5 to GND

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------------------------------------------------------------------
// Pin constants
// ---------------------------------------------------------------------------
const uint8_t BUTTON_ADD_PIN = 2;   // operator +
const uint8_t BUTTON_SUB_PIN = 3;   // operator -
const uint8_t BUTTON_MUL_PIN = 4;   // operator *
const uint8_t BUTTON_DIV_PIN = 5;   // operator /
const uint8_t OPERAND_A_PIN = A0;   // potentiometer for the first number
const uint8_t OPERAND_B_PIN = A1;   // potentiometer for the second number

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

const uint8_t NUM_BUTTONS = 4;
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {
  BUTTON_ADD_PIN, BUTTON_SUB_PIN, BUTTON_MUL_PIN, BUTTON_DIV_PIN
};
const uint8_t BTN_ADD = 0, BTN_SUB = 1, BTN_MUL = 2, BTN_DIV = 3;

const unsigned long DEBOUNCE_MS = 30;   // ignore shorter (noisy) pulses
const int MAX_OPERAND = 99;             // keeps every result inside 16 columns

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

int buttonState[NUM_BUTTONS];   // debounced state, LOW = pressed
int lastReading[NUM_BUTTONS];
unsigned long lastDebounceMs[NUM_BUTTONS];

int operandA = 0;               // value read from the A0 potentiometer (0..99)
int operandB = 0;               // value read from the A1 potentiometer (0..99)
int lastShownA = -1, lastShownB = -1;   // used to avoid pointless redraws

char lineBuffer[17];

// ---------------------------------------------------------------------------
// Read one potentiometer and convert it to 0..MAX_OPERAND.
// Four samples are averaged so the value does not jump around too much.
// ---------------------------------------------------------------------------
int readOperand(uint8_t pin) {
  long total = 0;
  for (uint8_t i = 0; i < 4; i++) {
    total += analogRead(pin);
    delayMicroseconds(200);
  }
  int raw = total / 4;                       // 0..1023
  return map(raw, 0, 1023, 0, MAX_OPERAND);  // 0..99
}

// ---------------------------------------------------------------------------
// Debounced button reader, returns true once per press
// ---------------------------------------------------------------------------
bool buttonPressed(uint8_t index) {
  bool newlyPressed = false;
  int reading = digitalRead(BUTTON_PINS[index]);

  if (reading != lastReading[index]) {
    lastDebounceMs[index] = millis();
  }
  lastReading[index] = reading;

  if ((millis() - lastDebounceMs[index]) >= DEBOUNCE_MS) {
    if (reading != buttonState[index]) {
      buttonState[index] = reading;
      if (buttonState[index] == LOW) newlyPressed = true;
    }
  }
  return newlyPressed;
}

// ---------------------------------------------------------------------------
// Show the two live operands on the bottom line
// ---------------------------------------------------------------------------
void showOperandsLine() {
  snprintf(lineBuffer, sizeof(lineBuffer), "A:%-2d  B:%-2d", operandA, operandB);
  lcd.setCursor(0, 1);
  lcd.print(lineBuffer);
}

// ---------------------------------------------------------------------------
// Do the requested calculation and print "a op b = result" on the top line
// ---------------------------------------------------------------------------
void calculate(uint8_t opIndex) {
  long result = 0;
  char opChar = '+';
  bool error = false;

  switch (opIndex) {
    case BTN_ADD: opChar = '+'; result = (long)operandA + operandB; break;
    case BTN_SUB: opChar = '-'; result = (long)operandA - operandB; break;
    case BTN_MUL: opChar = '*'; result = (long)operandA * operandB; break;
    case BTN_DIV:
      opChar = '/';
      if (operandB == 0) {
        error = true;              // integer division by zero is not allowed
      } else {
        result = operandA / operandB;   // integer division, the remainder is dropped
      }
      break;
  }

  lcd.setCursor(0, 0);
  if (error) {
    lcd.print(F("DIV BY ZERO!    "));   // 13 chars, padded to clear the line
    Serial.println(F("Error: division by zero"));
  } else {
    // 99 * 99 = 9801 -> "99 * 99 = 9801" fits comfortably in 16 columns
    snprintf(lineBuffer, sizeof(lineBuffer), "%d %c %d = %ld",
             operandA, opChar, operandB, result);
    lcd.print(lineBuffer);
    // clear anything left over from a longer previous result
    for (uint8_t i = strlen(lineBuffer); i < LCD_COLUMNS; i++) lcd.print(' ');

    Serial.print(F("Result: "));
    Serial.print(operandA);
    Serial.print(' ');
    Serial.print(opChar);
    Serial.print(' ');
    Serial.print(operandB);
    Serial.print(F(" = "));
    Serial.println(result);
  }

  showOperandsLine();   // keep the live operands visible underneath
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("045 - LCD Calculator starting..."));

  // Buttons use the internal pull-up: released = HIGH, pressed = LOW
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttonState[i] = digitalRead(BUTTON_PINS[i]);
    lastReading[i] = buttonState[i];
    lastDebounceMs[i] = 0;
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("LCD Calculator"));
  lcd.setCursor(0, 1);
  lcd.print(F("Press + - * /"));
  Serial.println(F("Turn the pots to set A and B, then press an operator button."));
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Press + - * /"));
}

void loop() {
  // --- Read the two potentiometers (operands) ----------------------------
  operandA = readOperand(OPERAND_A_PIN);
  operandB = readOperand(OPERAND_B_PIN);

  // Only refresh the bottom line when a value really changed
  if (operandA != lastShownA || operandB != lastShownB) {
    showOperandsLine();
    lastShownA = operandA;
    lastShownB = operandB;
    Serial.print(F("A = "));
    Serial.print(operandA);
    Serial.print(F(", B = "));
    Serial.println(operandB);
  }

  // --- Check the four operator buttons -----------------------------------
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (buttonPressed(i)) {
      calculate(i);
    }
  }

  delay(10);   // small, steady loop rate
}
