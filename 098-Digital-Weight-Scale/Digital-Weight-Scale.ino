/*
 * 098 - Digital Weight Scale
 * Board: Arduino Uno
 *
 * A digital scale built from a strain-gauge load cell and an HX711
 * amplifier/ADC. The measured weight is shown on a 16x2 I2C LCD in grams
 * (or kilograms above 1 kg) and printed to the Serial Monitor. Pressing the
 * button on pin 2 performs a TARE: everything currently on the scale
 * becomes the new zero, exactly like the tare button on a kitchen scale.
 *
 * Wiring:
 *   HX711 module VCC -> 5V, GND -> GND
 *   HX711 module DT  -> digital pin 4 (data)
 *   HX711 module SCK -> digital pin 5 (clock)
 *   Load cell red    -> E+, black -> E-, white -> A-, green -> A+
 *     (wire colours vary - check the diagram of your load cell)
 *   Tare push button -> digital pin 2 -> GND (internal pull-up used)
 *   LCD 1602 + I2C backpack: VCC -> 5V, GND -> GND, SDA -> A4, SCL -> A5
 *
 * Libraries (Library Manager): "HX711" by Bogdan Necula and
 * "LiquidCrystal I2C".
 *
 * ---------------- CALIBRATION (do this once) ----------------
 * 1. Upload this sketch with your CALIBRATION_FACTOR as it is and open the
 *    Serial Monitor (9600 baud).
 * 2. Put nothing on the scale and press the button on pin 2 to TARE it.
 * 3. Place a known weight on the scale (for example a 100 g bag of sugar).
 * 4. Read the value printed on the LCD/Serial, for example "87.0 g".
 * 5. Correct the factor with:   NEW = OLD * (shown / known)
 *    Here: NEW = OLD * (87.0 / 100.0)  -> put that number in
 *    CALIBRATION_FACTOR, re-upload and repeat steps 2-4 until a 100 g
 *    weight really shows 100.0 g. A negative value is normal: it only
 *    means the load cell wires are the other way round.
 */

#include <HX711.h>                 // HX711 library by Bogdan Necula
#include <Wire.h>                  // built-in I2C library
#include <LiquidCrystal_I2C.h>     // 16x2 I2C LCD

// ---------------- Named pin constants ----------------
const int HX711_DOUT_PIN = 4;   // HX711 data output
const int HX711_SCK_PIN = 5;    // HX711 clock
const int TARE_BUTTON_PIN = 2;  // press to zero the scale
const int LCD_ADDRESS = 0x27;   // 0x3F on some backpacks
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;

// ---------------- Calibration ----------------
// Replace this starting value with your own measurement - see the CALIBRATION
// steps in the header comment above.
const float CALIBRATION_FACTOR = -7050.0;

const unsigned long DEBOUNCE_MS = 40;
const unsigned long READ_TIMEOUT_MS = 500;  // give up if the HX711 is silent
const int AVERAGE_READINGS = 5;             // readings averaged per sample

HX711 scale;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

float currentGrams = 0.0;
int readingCount = 0;

// Debounce bookkeeping for the tare button
int lastRawButton = HIGH;
int lastStableButton = HIGH;
unsigned long lastButtonChangeMs = 0;

// Debounced press: true once per clean press of the tare button.
bool tarePressed() {
  int raw = digitalRead(TARE_BUTTON_PIN);
  unsigned long now = millis();
  if (raw != lastRawButton) {
    lastRawButton = raw;
    lastButtonChangeMs = now;
  }
  if (now - lastButtonChangeMs >= DEBOUNCE_MS && raw != lastStableButton) {
    lastStableButton = raw;
    if (raw == LOW) return true;
  }
  return false;
}

// Write the weight on the LCD, choosing grams or kilograms automatically.
// dtostrf() is used instead of printf-style %f because the standard Arduino
// sprintf() on the Uno does not format floating point numbers.
void updateDisplay(float grams) {
  char value[17];
  if (grams >= 1000.0 || grams <= -1000.0) {
    dtostrf(grams / 1000.0, 8, 2, value);   // at least 8 chars, 2 decimals
    strcat(value, " kg");
  } else {
    dtostrf(grams, 8, 1, value);            // at least 8 chars, 1 decimal
    strcat(value, " g");
  }

  lcd.setCursor(0, 0);
  lcd.print(value);
  lcd.setCursor(0, 1);
  lcd.print("TARE: press btn2");
}

void setup() {
  Serial.begin(9600);
  pinMode(TARE_BUTTON_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Digital Scale");
  lcd.setCursor(0, 1);
  lcd.print("Warming up...");

  Serial.println("Digital Weight Scale starting...");

  // Start the HX711 on the two named pins.
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);   // apply the calibration constant

  // Give the load cell a moment to settle, then set the current reading to 0.
  delay(2000);
  scale.tare(AVERAGE_READINGS);
  Serial.println("Tared with an empty platform.");
  Serial.print("CALIBRATION_FACTOR = ");
  Serial.println(CALIBRATION_FACTOR, 1);
  Serial.println("Place an object of known weight and compare the reading.");
}

void loop() {
  // ---- 1. Tare button: make whatever is on the scale the new zero ----
  if (tarePressed()) {
    scale.tare(AVERAGE_READINGS);
    currentGrams = 0.0;
    Serial.println("TARE - scale reset to 0.0 g.");
    updateDisplay(currentGrams);
  }

  // ---- 2. Read the HX711 (wait_ready_timeout avoids hanging if it is
  //         unplugged, instead the sketch keeps running and says so) ----
  if (scale.wait_ready_timeout(READ_TIMEOUT_MS)) {
    float grams = scale.get_units(AVERAGE_READINGS);
    currentGrams = grams;
    readingCount++;

    updateDisplay(grams);

    Serial.print("Reading #");
    Serial.print(readingCount);
    Serial.print(": ");
    Serial.print(grams, 1);
    Serial.print(" g  (");
    Serial.print(grams / 1000.0, 3);
    Serial.println(" kg)");
  } else {
    Serial.println("HX711 not responding - check DT -> pin 4, SCK -> pin 5.");
    lcd.setCursor(0, 0);
    lcd.print("  NO HX711    ");
  }

  delay(200);   // a new display/Serial line about five times per second
}
