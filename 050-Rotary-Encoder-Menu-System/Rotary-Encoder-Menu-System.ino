// 050 - Rotary Encoder Menu System
// A KY-040 rotary encoder browses a menu on a 16x2 I2C LCD. Turning the knob
// moves the cursor, pressing it selects an item, and turning again adjusts the
// value of that item (LED brightness, buzzer tone) or opens an ABOUT screen.
// Board: Arduino Uno
//
// Library: LiquidCrystal I2C (by Frank de Brabander)
//
// Wiring:
//   KY-040 encoder -> 5V, GND
//        CLK -> pin 2 (this pin supports external interrupts on the Uno)
//        DT  -> pin 3
//        SW  -> pin 4 (internal pull-up is used, other side to GND)
//   LED (any colour) -> pin 5 through a 220 ohm resistor to GND
//   Piezo buzzer (+) -> pin 6, buzzer (-) -> GND
//   LCD 1602 (I2C 0x27) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------------------------------------------------------------------
// Pin constants
// ---------------------------------------------------------------------------
const uint8_t ENCODER_CLK_PIN = 2;   // clock channel, drives the interrupt
const uint8_t ENCODER_DT_PIN = 3;    // direction channel
const uint8_t ENCODER_SW_PIN = 4;    // push button of the encoder
const uint8_t LED_PIN = 5;           // PWM output that the menu controls
const uint8_t BUZZER_PIN = 6;        // tone() output that the menu controls

const uint8_t LCD_ADDRESS = 0x27;
const uint8_t LCD_COLUMNS = 16;
const uint8_t LCD_ROWS = 2;

// ---------------------------------------------------------------------------
// Menu and value constants
// ---------------------------------------------------------------------------
const char* const MENU_ITEMS[] = {"LED BRIGHTNESS", "BUZZER TONE", "ABOUT"};
const uint8_t MENU_COUNT = 3;
const uint8_t ITEM_BRIGHTNESS = 0;
const uint8_t ITEM_TONE = 1;
const uint8_t ITEM_ABOUT = 2;

const uint8_t BRIGHTNESS_MIN = 0;
const uint8_t BRIGHTNESS_MAX = 255;
const uint8_t BRIGHTNESS_STEP = 5;
const uint8_t BRIGHTNESS_DEFAULT = 128;

const int TONE_MIN = 100;            // Hz
const int TONE_MAX = 5000;           // Hz
const int TONE_STEP = 100;           // Hz per encoder click while adjusting
const int TONE_DEFAULT = 1000;       // Hz

const unsigned long DEBOUNCE_MS = 30;

// ---------------------------------------------------------------------------
// Program states
// ---------------------------------------------------------------------------
enum MenuMode { MODE_BROWSE, MODE_ADJUST, MODE_ABOUT };

MenuMode mode = MODE_BROWSE;
uint8_t menuIndex = 0;               // which menu entry the cursor points at

uint8_t brightness = BRIGHTNESS_DEFAULT;
int toneFrequency = TONE_DEFAULT;

volatile long encoderPosition = 0;   // updated inside the interrupt
long lastEncoderPosition = 0;        // value used during the previous loop

int buttonState = HIGH;              // debounced state, LOW = pressed
int lastReading = HIGH;
unsigned long lastDebounceMs = 0;

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// ---------------------------------------------------------------------------
// Interrupt service routine: called every time the CLK pin falls.
// If the encoder turns the wrong way for your module, swap the two branches
// of the if() statement below.
// ---------------------------------------------------------------------------
void encoderISR() {
  if (digitalRead(ENCODER_DT_PIN)) {
    encoderPosition++;      // one direction
  } else {
    encoderPosition--;      // the other direction
  }
}

// ---------------------------------------------------------------------------
// Debounced press of the encoder push button, true once per press
// ---------------------------------------------------------------------------
bool buttonPressed() {
  bool newlyPressed = false;
  int reading = digitalRead(ENCODER_SW_PIN);

  if (reading != lastReading) lastDebounceMs = millis();
  lastReading = reading;

  if ((millis() - lastDebounceMs) >= DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) newlyPressed = true;
    }
  }
  return newlyPressed;
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------
void printLine(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  lcd.print(text);
  for (uint8_t i = strlen(text); i < LCD_COLUMNS; i++) lcd.print(' ');
}

void showBrowse() {
  char line[17];
  snprintf(line, sizeof(line), ">%s", MENU_ITEMS[menuIndex]);   // '>' = cursor
  printLine(0, line);
  printLine(1, "Click to select");
  Serial.print(F("Menu: "));
  Serial.println(MENU_ITEMS[menuIndex]);
}

void showAdjust() {
  char line[17];
  printLine(0, MENU_ITEMS[menuIndex]);
  if (menuIndex == ITEM_BRIGHTNESS) {
    snprintf(line, sizeof(line), "Value: %3u", (unsigned int)brightness);
    Serial.print(F("Brightness = "));
    Serial.println(brightness);
  } else {
    snprintf(line, sizeof(line), "Value: %4d Hz", toneFrequency);
    Serial.print(F("Tone = "));
    Serial.print(toneFrequency);
    Serial.println(F(" Hz"));
  }
  printLine(1, line);
}

void showAbout() {
  printLine(0, "050 Menu System");
  printLine(1, "Uno + KY-040");
  Serial.println(F("ABOUT screen opened"));
}

// ---------------------------------------------------------------------------
// Apply the current values to the hardware
// ---------------------------------------------------------------------------
void applyBrightness() {
  analogWrite(LED_PIN, brightness);   // PWM 0..255
}

void applyTone() {
  // short beep so the user can hear the new frequency immediately
  tone(BUZZER_PIN, toneFrequency, 60);
}

// ---------------------------------------------------------------------------
// React to a rotation of the knob
// ---------------------------------------------------------------------------
void handleRotation(long delta) {
  switch (mode) {
    case MODE_BROWSE: {
      // move the cursor, wrapping around at both ends
      long next = (long)menuIndex + delta;
      next = (next % MENU_COUNT + MENU_COUNT) % MENU_COUNT;   // always positive
      menuIndex = (uint8_t)next;
      showBrowse();
      break;
    }
    case MODE_ADJUST:
      if (menuIndex == ITEM_BRIGHTNESS) {
        long v = (long)brightness + delta * BRIGHTNESS_STEP;
        brightness = (uint8_t)constrain(v, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        applyBrightness();
      } else {
        long v = (long)toneFrequency + delta * TONE_STEP;
        toneFrequency = (int)constrain(v, TONE_MIN, TONE_MAX);
        applyTone();
      }
      showAdjust();
      break;
    case MODE_ABOUT:
      break;   // nothing to scroll here
  }
}

// ---------------------------------------------------------------------------
// React to a press of the encoder button
// ---------------------------------------------------------------------------
void handleClick() {
  switch (mode) {
    case MODE_BROWSE:
      if (menuIndex == ITEM_ABOUT) {
        mode = MODE_ABOUT;
        showAbout();
      } else {
        mode = MODE_ADJUST;
        showAdjust();
        Serial.println(F("Adjust mode - turn the knob to change the value"));
      }
      break;

    case MODE_ADJUST:
      mode = MODE_BROWSE;
      noTone(BUZZER_PIN);           // stop any tone before leaving
      showBrowse();
      Serial.println(F("Value saved"));
      break;

    case MODE_ABOUT:
      mode = MODE_BROWSE;
      showBrowse();
      break;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("050 - Rotary Encoder Menu System starting..."));

  pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  buttonState = digitalRead(ENCODER_SW_PIN);
  lastReading = buttonState;

  // Attach the interrupt: pin 2 is one of the two pins that support it on Uno
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderISR, FALLING);

  // Start the LCD and show a short splash screen
  lcd.init();
  lcd.backlight();
  lcd.clear();
  printLine(0, "Rotary Encoder");
  printLine(1, "Menu System");
  delay(1200);

  applyBrightness();     // light the LED at the default level
  showBrowse();

  Serial.println(F("Turn = browse / adjust, press = select"));
}

void loop() {
  // --- Read the encoder position safely (4 byte value, disable IRQ briefly)
  noInterrupts();
  long position = encoderPosition;
  interrupts();

  long delta = position - lastEncoderPosition;
  if (delta != 0) {
    lastEncoderPosition = position;
    handleRotation(delta);
  }

  // --- Read the encoder push button --------------------------------------
  if (buttonPressed()) {
    handleClick();
  }
}
