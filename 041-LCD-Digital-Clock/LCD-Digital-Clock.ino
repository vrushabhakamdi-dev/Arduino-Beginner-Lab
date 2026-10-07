// 041 - LCD Digital Clock
// A real-time clock that shows HH:MM:SS plus the weekday and date on a 16x2 I2C LCD.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - RTClib                  (by Adafruit)
//   - LiquidCrystal I2C       (by Frank de Brabander)
//
// Wiring:
//   DS3231 RTC module  ->  SDA = A4, SCL = A5, VCC = 5V, GND = GND
//   LCD 1602 (0x27)    ->  SDA = A4, SCL = A5, VCC = 5V, GND = GND
//   (both modules share the same two I2C wires, that is normal)

#include <Wire.h>                 // I2C bus used by the RTC and the LCD
#include <RTClib.h>               // Adafruit RTClib -> reads the DS3231
#include <LiquidCrystal_I2C.h>   // Frank de Brabander -> drives the I2C LCD

// ---------------------------------------------------------------------------
// Device constants (named constants instead of magic numbers)
// ---------------------------------------------------------------------------
const uint8_t LCD_ADDRESS = 0x27;  // I2C address of the LCD backpack (some are 0x3F)
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

const unsigned long REFRESH_MS = 250;  // how often the display refreshes (4 times a second)

// Day and month names used when building the date line
const char* const DAY_NAMES[7] =
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};      // index 0 = Sunday
const char* const MONTH_NAMES[12] =
  {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// ---------------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------------
RTC_DS3231 rtc;                          // the DS3231 real-time clock chip
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);  // the 16x2 display

char lineBuffer[17];  // 16 characters + one space for the string terminator

// ---------------------------------------------------------------------------
// helper: print a line on the LCD and wipe whatever was left over
// ---------------------------------------------------------------------------
void printLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  // pad with spaces so old characters disappear when the new text is shorter
  for (uint8_t i = strlen(text); i < LCD_COLUMNS; i++) {
    lcd.print(' ');
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("041 - LCD Digital Clock starting..."));

  // --- Start the LCD and show a short boot message -----------------------
  lcd.init();          // initialise the I2C LCD
  lcd.backlight();     // turn the backlight on
  printLine(0, "RTC Clock booting");
  printLine(1, "please wait...");
  delay(800);

  // --- Start the DS3231 RTC ---------------------------------------------
  if (!rtc.begin()) {
    Serial.println(F("ERROR: DS3231 RTC not found – check SDA/SCL/VCC/GND!"));
    printLine(0, "RTC not found!");
    printLine(1, "Check wiring");
    while (true) { delay(1000); }   // nothing else can work without the RTC
  }
  Serial.println(F("DS3231 RTC found."));

  // *** HOW TO SET THE INITIAL TIME ***
  // If the module never had power (new board or the coin cell was removed)
  // rtc.lostPower() returns true and we seed the clock.
  // The line below uses the moment this sketch was COMPILED, so:
  //   1) set your PC clock to the correct local time
  //   2) press Verify/Upload  -> the clock starts at that exact time
  // To set a fixed time by hand instead, replace it with e.g.:
  //     rtc.adjust(DateTime(2026, 10, 7, 19, 30, 0)); // year, month, day, hh, mm, ss
  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power – setting time from compile time."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.clear();
}

void loop() {
  DateTime now = rtc.now();   // read the current date and time from the RTC

  // --- Line 0: time as HH:MM:SS -----------------------------------------
  // %02d prints the number padded with a leading zero, so 7 becomes "07"
  snprintf(lineBuffer, sizeof(lineBuffer), "Time %02d:%02d:%02d",
           now.hour(), now.minute(), now.second());
  printLine(0, lineBuffer);

  // --- Line 1: weekday and date, for example "Tue 07 Oct 2026" ----------
  snprintf(lineBuffer, sizeof(lineBuffer), "%s %02d %s %d",
           DAY_NAMES[now.dayOfTheWeek()],   // 0 = Sunday ... 6 = Saturday
           now.day(),
           MONTH_NAMES[now.month() - 1],    // months are 1..12, array is 0..11
           now.year());
  printLine(1, lineBuffer);

  // --- Mirror everything to the Serial Monitor ---------------------------
  Serial.print(F("Time: "));
  Serial.print(now.hour());
  Serial.print(':');
  if (now.minute() < 10) Serial.print('0');
  Serial.print(now.minute());
  Serial.print(':');
  if (now.second() < 10) Serial.print('0');
  Serial.print(now.second());
  Serial.print(F("  Date: "));
  Serial.print(DAY_NAMES[now.dayOfTheWeek()]);
  Serial.print(' ');
  Serial.print(now.day());
  Serial.print(' ');
  Serial.print(MONTH_NAMES[now.month() - 1]);
  Serial.print(' ');
  Serial.println(now.year());

  delay(REFRESH_MS);   // refresh 4x per second so the seconds tick smoothly
}
