/*
 * 097 - Electronic Queue Token System
 * Board: Arduino Uno
 *
 * A small queue-management terminal, like the ticket machines in banks or
 * clinics. Pressing the ISSUE button on pin 2 hands out the next token and
 * increments the counter. Pressing the CALL NEXT button on pin 3 moves the
 * "now serving" number forward and rings the buzzer on pin 8 so the next
 * person knows it is their turn. A 16x2 I2C LCD always shows:
 *
 *   LINE 1: TOKEN: 007
 *   LINE 2: NOW SERVING: 005
 *
 * Every issue and call event is also printed to the Serial Monitor with a
 * timestamp, so a whole session can be replayed from the log.
 *
 * Wiring:
 *   Push button ISSUE     -> digital pin 2 -> GND (internal pull-up used)
 *   Push button CALL NEXT -> digital pin 3 -> GND (internal pull-up used)
 *   LCD 1602 + I2C backpack: VCC -> 5V, GND -> GND, SDA -> A4, SCL -> A5
 *     (default I2C address 0x27 - see the notes if yours is 0x3F)
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *
 * Library: install "LiquidCrystal I2C" from the Library Manager.
 */

#include <Wire.h>               // built-in I2C library
#include <LiquidCrystal_I2C.h>  // drives the 16x2 LCD over I2C

// ---------------- Named pin constants ----------------
const int ISSUE_BUTTON_PIN = 2;   // hand out a new token
const int CALL_BUTTON_PIN = 3;    // call the next waiting token
const int BUZZER_PIN = 8;         // rings when a token is called
const int LCD_ADDRESS = 0x27;     // common address; 0x3F is the other one
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;

const unsigned long DEBOUNCE_MS = 40;
const unsigned long CALL_RING_MS = 300;   // length of one ring tone
const int MAX_TOKEN = 999;                // the display shows three digits

// ---------------- Queue state ----------------
int tokensIssued = 0;   // highest token handed out so far
int nowServing = 0;     // token currently at the counter

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Debounce bookkeeping, one slot per button (0 = issue, 1 = call)
const int BUTTON_PINS[2] = {ISSUE_BUTTON_PIN, CALL_BUTTON_PIN};
int lastRaw[2] = {HIGH, HIGH};
int lastStable[2] = {HIGH, HIGH};
unsigned long lastChangeMs[2] = {0, 0};
bool pressEvent[2] = {false, false};

// Prints [MM:SS] so log lines can be lined up with the session timeline.
void printTimestamp() {
  unsigned long totalSeconds = millis() / 1000;
  Serial.print('[');
  if (totalSeconds / 60 < 10) Serial.print('0');
  Serial.print(totalSeconds / 60);
  Serial.print(':');
  if (totalSeconds % 60 < 10) Serial.print('0');
  Serial.print(totalSeconds % 60);
  Serial.print("] ");
}

// Poll both buttons and raise pressEvent[i] once per clean press.
void readButtons() {
  unsigned long now = millis();
  for (int i = 0; i < 2; i++) {
    int raw = digitalRead(BUTTON_PINS[i]);
    if (raw != lastRaw[i]) {
      lastRaw[i] = raw;
      lastChangeMs[i] = now;
    }
    if (now - lastChangeMs[i] >= DEBOUNCE_MS && raw != lastStable[i]) {
      lastStable[i] = raw;
      if (raw == LOW) pressEvent[i] = true;
    }
  }
}

// Refresh the LCD only when a value changed - this avoids visible flicker.
void updateDisplay() {
  static int shownIssued = -1;
  static int shownServing = -1;
  if (shownIssued == tokensIssued && shownServing == nowServing) return;

  shownIssued = tokensIssued;
  shownServing = nowServing;

  char line1[17];
  char line2[17];
  // %03d pads the numbers with leading zeros: 7 becomes "007".
  snprintf(line1, sizeof(line1), "TOKEN: %03d", tokensIssued);
  snprintf(line2, sizeof(line2), "NOW SERVING: %03d", nowServing);

  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void issueToken() {
  if (tokensIssued >= MAX_TOKEN) {
    printTimestamp();
    Serial.println("Issue rejected - token counter is full (999).");
    return;
  }
  tokensIssued++;
  printTimestamp();
  Serial.print("TOKEN ISSUED: ");
  Serial.print(tokensIssued);
  Serial.print(" | waiting: ");
  Serial.println(tokensIssued - nowServing);
  updateDisplay();
}

void callNext() {
  if (nowServing >= tokensIssued) {
    // Nobody is waiting: one short low tone as feedback, no ring.
    printTimestamp();
    Serial.println("Call ignored - no tokens are waiting.");
    tone(BUZZER_PIN, 400);
    delay(150);
    noTone(BUZZER_PIN);
    return;
  }

  nowServing++;
  printTimestamp();
  Serial.print("NOW SERVING: ");
  Serial.print(nowServing);
  Serial.print(" | still waiting: ");
  Serial.println(tokensIssued - nowServing);
  updateDisplay();

  // The queue bell: three rings so it is heard over background noise.
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 2000);
    delay(CALL_RING_MS);
    noTone(BUZZER_PIN);
    delay(120);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ISSUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CALL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  lcd.init();       // start the I2C LCD
  lcd.backlight();  // switch its backlight on
  updateDisplay();  // show TOKEN: 000 / NOW SERVING: 000

  Serial.println("Electronic Queue Token System ready.");
  Serial.println("Button pin 2 = ISSUE TOKEN, button pin 3 = CALL NEXT.");
}

void loop() {
  readButtons();

  if (pressEvent[0]) {
    pressEvent[0] = false;
    issueToken();
  }
  if (pressEvent[1]) {
    pressEvent[1] = false;
    callNext();
  }

  delay(10);
}
