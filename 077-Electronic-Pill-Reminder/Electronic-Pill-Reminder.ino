// 077 - Electronic Pill Reminder
// A 16x2 I2C LCD shows the next medicine dose. When the dose time arrives a
// buzzer and an LED keep signalling until the patient presses a button.
// Board: Arduino Uno
// Wiring:
//   LCD I2C (PCF8574, address 0x27): SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND
//   Acknowledge button -> pin 2 to GND (INPUT_PULLUP is used)
//   Buzzer -> pin 8, LED -> pin 13 through a 220 ohm resistor
//
// Libraries: LiquidCrystal I2C (install from the Library Manager)

#include <LiquidCrystal_I2C.h>  // Drives a 1602 LCD through an I2C backpack

// ---------------- LCD settings ----------------
const byte LCD_ADDRESS = 0x27;  // Common address; try 0x3F if the screen stays dark
const byte LCD_COLUMNS = 16;
const byte LCD_ROWS = 2;

// ---------------- Pin constants ----------------
const int BUTTON_PIN = 2;   // Acknowledge button
const int BUZZER_PIN = 8;   // Buzzer sounds while a dose is waiting
const int LED_PIN = 13;     // LED flashes while a dose is waiting

// ---------------- The three dose times (change these to suit the patient) ----------------
const byte DOSE_COUNT = 3;
const byte DOSE_HOUR[DOSE_COUNT] = {8, 13, 20};   // 08:00, 13:30, 20:00
const byte DOSE_MINUTE[DOSE_COUNT] = {0, 30, 0};

// The time of day shown by the running clock when the board is switched on.
// Set it a few minutes before the first dose to see the alarm quickly in a demo.
const byte START_HOUR = 7;
const byte START_MINUTE = 55;

// ---------------- Timing constants ----------------
const unsigned long DEBOUNCE_MS = 50;     // Button debounce time
const unsigned long ALARM_BLINK_MS = 300; // Alarm blinks about 3 times a second

// ---------------- State variables ----------------
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

bool doseTaken[DOSE_COUNT] = {false, false, false};
bool alarmActive = false;
int currentDay = -1;             // Changes when the clock passes midnight
unsigned long secondsOfDay = 0;  // Seconds since 00:00 of the running clock
unsigned long lastBlinkMs = 0;
bool blinkOn = false;
int lastRenderedSecond = -1;     // Redraw the LCD only when the second changes

int lastButtonReading = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceMs = 0;

// ---------------- Clock helpers ----------------
// The clock is built from three compile-time constants plus millis()
void updateClock() {
  unsigned long elapsedSeconds = millis() / 1000UL;
  unsigned long total = (START_HOUR * 3600UL) + (START_MINUTE * 60UL) + elapsedSeconds;

  int day = (int)(total / 86400UL);
  secondsOfDay = total % 86400UL;

  if (day != currentDay) {
    currentDay = day;
    for (byte i = 0; i < DOSE_COUNT; i++) doseTaken[i] = false;  // New day = new doses
    Serial.print("New day started (day ");
    Serial.print(currentDay);
    Serial.println(") - dose list reset.");
  }
}

unsigned long doseSeconds(byte index) {
  return (unsigned long)DOSE_HOUR[index] * 3600UL + (unsigned long)DOSE_MINUTE[index] * 60UL;
}

bool doseIsDue(byte index) {
  return !doseTaken[index] && secondsOfDay >= doseSeconds(index);
}

// Find the next dose that has not been taken yet and the time left until it
bool findNextDose(byte *indexOut, unsigned long *secondsLeftOut, unsigned long *timeOfDayOut) {
  unsigned long bestSecondsLeft = 999999999UL;
  bool found = false;

  for (byte i = 0; i < DOSE_COUNT; i++) {
    if (doseTaken[i]) continue;

    unsigned long secondsLeft;
    unsigned long timeOfDay = doseSeconds(i);
    if (timeOfDay > secondsOfDay) {
      secondsLeft = timeOfDay - secondsOfDay;          // Later today
    } else {
      secondsLeft = timeOfDay + 86400UL - secondsOfDay; // Already past - tomorrow
    }
    if (secondsLeft < bestSecondsLeft) {
      bestSecondsLeft = secondsLeft;
      *indexOut = i;
      *secondsLeftOut = secondsLeft;
      *timeOfDayOut = timeOfDay;
      found = true;
    }
  }
  return found;
}

// Print hh:mm from an hour/minute pair
void printClockTime(byte h, byte m) {
  if (h < 10) Serial.print('0');
  Serial.print(h);
  Serial.print(':');
  if (m < 10) Serial.print('0');
  Serial.print(m);
}

// ---------------- LCD rendering ----------------
void renderLcd() {
  byte idx = 0;
  unsigned long secondsLeft = 0;
  unsigned long timeOfDay = 0;
  bool hasNext = findNextDose(&idx, &secondsLeft, &timeOfDay);

  if (alarmActive) {
    // Show the dose that is actually waiting right now (the first one due today)
    byte dueIdx = 0;
    bool foundDue = false;
    for (byte i = 0; i < DOSE_COUNT; i++) {
      if (doseIsDue(i)) {
        dueIdx = i;
        foundDue = true;
        break;
      }
    }
    if (!foundDue) {  // Fallback: show the next scheduled dose
      findNextDose(&dueIdx, &secondsLeft, &timeOfDay);
    } else {
      timeOfDay = doseSeconds(dueIdx);
    }

    lcd.setCursor(0, 0);
    lcd.print("TAKE PILL NOW!  ");
    lcd.setCursor(0, 1);
    lcd.print("Dose ");
    byte h = (byte)(timeOfDay / 3600UL);
    byte m = (byte)((timeOfDay % 3600UL) / 60UL);
    if (h < 10) lcd.print('0');
    lcd.print(h);
    lcd.print(':');
    if (m < 10) lcd.print('0');
    lcd.print(m);
    lcd.print("   BTN");   // "Dose hh:mm   BTN" fills the 16 columns exactly
  } else if (hasNext) {
    // Line 1: the label plus the time of the next dose
    lcd.setCursor(0, 0);
    lcd.print("NEXT DOSE       ");
    lcd.setCursor(10, 0);
    byte h = (byte)(timeOfDay / 3600UL);
    byte m = (byte)((timeOfDay % 3600UL) / 60UL);
    if (h < 10) lcd.print('0');
    lcd.print(h);
    lcd.print(':');
    if (m < 10) lcd.print('0');
    lcd.print(m);

    // Line 2: live countdown until that dose
    lcd.setCursor(0, 1);
    lcd.print("IN ");
    unsigned long hours = secondsLeft / 3600UL;
    unsigned long minutes = (secondsLeft % 3600UL) / 60UL;
    unsigned long seconds = secondsLeft % 60UL;
    if (hours < 10) lcd.print('0');
    lcd.print(hours);
    lcd.print(':');
    if (minutes < 10) lcd.print('0');
    lcd.print(minutes);
    lcd.print(':');
    if (seconds < 10) lcd.print('0');
    lcd.print(seconds);
    lcd.print("     ");   // Pad the line to 16 columns
  } else {
    // Every dose for today has been taken - show tomorrow's first dose
    lcd.setCursor(0, 0);
    lcd.print("ALL DOSES DONE  ");
    lcd.setCursor(0, 1);
    lcd.print("TOMORROW ");
    if (DOSE_HOUR[0] < 10) lcd.print('0');
    lcd.print(DOSE_HOUR[0]);
    lcd.print(':');
    if (DOSE_MINUTE[0] < 10) lcd.print('0');
    lcd.print(DOSE_MINUTE[0]);
    lcd.print("  ");   // "TOMORROW hh:mm  " = 16 columns
  }
}

// ---------------- Alarm control ----------------
void startAlarm() {
  alarmActive = true;
  blinkOn = true;
  lastBlinkMs = millis();
  tone(BUZZER_PIN, 2000);
  digitalWrite(LED_PIN, HIGH);

  Serial.print("DOSE DUE at ");
  byte h = (byte)(secondsOfDay / 3600UL);
  byte m = (byte)((secondsOfDay % 3600UL) / 60UL);
  printClockTime(h, m);
  Serial.println(" - buzzer and LED ON, waiting for the button.");
}

void acknowledgeDose() {
  // Mark every dose that is already due as taken
  for (byte i = 0; i < DOSE_COUNT; i++) {
    if (doseIsDue(i)) {
      doseTaken[i] = true;
      Serial.print("Dose ");
      printClockTime(DOSE_HOUR[i], DOSE_MINUTE[i]);
      Serial.println(" acknowledged by the patient.");
    }
  }
  alarmActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Reminder stopped. Next dose armed.");
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  lcd.init();        // Start the LCD (part of the LiquidCrystal I2C library)
  lcd.backlight();   // Turn the backlight on
  lcd.setCursor(0, 0);
  lcd.print("Pill Reminder");
  lcd.setCursor(0, 1);
  lcd.print("Loading...");

  Serial.begin(9600);
  Serial.println("Electronic Pill Reminder starting.");
  Serial.print("Doses today: ");
  for (byte i = 0; i < DOSE_COUNT; i++) {
    printClockTime(DOSE_HOUR[i], DOSE_MINUTE[i]);
    if (i < DOSE_COUNT - 1) Serial.print(", ");
  }
  Serial.println();
  Serial.print("Clock starts at ");
  if (START_HOUR < 10) Serial.print('0');
  Serial.print(START_HOUR);
  Serial.print(':');
  if (START_MINUTE < 10) Serial.print('0');
  Serial.println(START_MINUTE);

  updateClock();
  renderLcd();
}

void loop() {
  unsigned long now = millis();
  updateClock();

  // ----- 1. Has a dose time arrived? -----
  if (!alarmActive) {
    for (byte i = 0; i < DOSE_COUNT; i++) {
      if (doseIsDue(i)) {
        startAlarm();
        renderLcd();
        break;
      }
    }
  }

  // ----- 2. Flash the LED and beep the buzzer while the dose is pending -----
  if (alarmActive && now - lastBlinkMs >= ALARM_BLINK_MS) {
    lastBlinkMs = now;
    blinkOn = !blinkOn;
    digitalWrite(LED_PIN, blinkOn ? HIGH : LOW);
    if (blinkOn) tone(BUZZER_PIN, 2000);
    else noTone(BUZZER_PIN);
  }

  // ----- 3. Read and debounce the acknowledge button -----
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonReading) lastDebounceMs = now;
  lastButtonReading = reading;

  if ((now - lastDebounceMs) > DEBOUNCE_MS && reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {  // Pressed
      if (alarmActive) {
        acknowledgeDose();
        renderLcd();
      } else {
        Serial.println("Button pressed - no dose is waiting right now.");
      }
    }
  }

  // ----- 4. Refresh the LCD once per second -----
  int secondNow = (int)(secondsOfDay % 60UL);
  if (secondNow != lastRenderedSecond) {
    lastRenderedSecond = secondNow;
    renderLcd();

    // Also keep a small clock line on the Serial Monitor
    if (!alarmActive) {
      Serial.print("Clock ");
      printClockTime((byte)(secondsOfDay / 3600UL), (byte)((secondsOfDay % 3600UL) / 60UL));
      Serial.println(" - waiting for the next dose.");
    }
  }

  delay(20);
}
