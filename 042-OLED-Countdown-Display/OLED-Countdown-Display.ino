// 042 - OLED Countdown Display
// A countdown timer with big mm:ss digits and a shrinking progress bar on a
// 128x64 SSD1306 OLED. Buttons set the minutes and start/stop the countdown.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - Adafruit SSD1306
//   - Adafruit GFX Library
//
// Wiring:
//   SSD1306 OLED 128x64 (I2C 0x3C) -> SDA = A4, SCL = A5, VCC = 5V, GND = GND
//   Button "MIN +" -> pin 2 to GND (uses internal pull-up)
//   Button "MIN -" -> pin 3 to GND
//   Button START/STOP -> pin 4 to GND

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Pin and display constants
// ---------------------------------------------------------------------------
const uint8_t BUTTON_MIN_PLUS_PIN = 2;    // press = add one minute
const uint8_t BUTTON_MIN_MINUS_PIN = 3;   // press = remove one minute
const uint8_t BUTTON_START_STOP_PIN = 4;  // press = start or pause the timer

const uint8_t NUM_BUTTONS = 3;
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {
  BUTTON_MIN_PLUS_PIN, BUTTON_MIN_MINUS_PIN, BUTTON_START_STOP_PIN
};
const uint8_t BTN_PLUS = 0;      // index inside BUTTON_PINS
const uint8_t BTN_MINUS = 1;
const uint8_t BTN_START = 2;

const unsigned long DEBOUNCE_MS = 30;  // ignore shorter pulses (switch noise)

const uint8_t SCREEN_WIDTH = 128;      // OLED width in pixels
const uint8_t SCREEN_HEIGHT = 64;      // OLED height in pixels
const uint8_t OLED_ADDRESS = 0x3C;     // most 128x64 SSD1306 modules use 0x3C

const uint8_t MIN_MINUTES = 1;         // smallest selectable countdown
const uint8_t MAX_MINUTES = 99;        // largest selectable countdown
const uint8_t START_MINUTES = 5;       // power-up default = 5 minutes

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int buttonState[NUM_BUTTONS];        // debounced state, LOW = pressed
int lastReading[NUM_BUTTONS];        // raw reading of the previous loop
unsigned long lastDebounceMs[NUM_BUTTONS];

uint8_t minutes = START_MINUTES;     // minutes currently selected
long totalSeconds = (long)START_MINUTES * 60;  // full length of the current run
long remainingSeconds = totalSeconds;           // seconds still to go
bool running = false;                // is the countdown ticking?
bool displayDirty = true;            // set to true whenever the OLED must be redrawn

unsigned long lastTickMs = 0;        // millis() of the last whole second tick

// ---------------------------------------------------------------------------
// Debounced button reader.
// Returns true exactly once, on the moment the button is pushed down.
// ---------------------------------------------------------------------------
bool buttonPressed(uint8_t index) {
  bool newlyPressed = false;
  int reading = digitalRead(BUTTON_PINS[index]);

  // any change restarts the debounce timer
  if (reading != lastReading[index]) {
    lastDebounceMs[index] = millis();
  }
  lastReading[index] = reading;

  // accept the reading only if it has been stable long enough
  if ((millis() - lastDebounceMs[index]) >= DEBOUNCE_MS) {
    if (reading != buttonState[index]) {
      buttonState[index] = reading;
      if (buttonState[index] == LOW) {   // buttons are wired to GND
        newlyPressed = true;
      }
    }
  }
  return newlyPressed;
}

// ---------------------------------------------------------------------------
// Split a number of seconds into "mm:ss" and write it at the given position
// ---------------------------------------------------------------------------
void drawBigTime(long seconds, int16_t x, int16_t y) {
  long m = seconds / 60;
  long s = seconds % 60;
  display.setTextSize(4);            // 4x = 24 px wide and 32 px tall characters
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  if (m < 10) display.print('0');
  display.print(m);
  display.print(':');
  if (s < 10) display.print('0');
  display.print(s);
}

// ---------------------------------------------------------------------------
// Redraw the whole screen: big time, status word and progress bar
// ---------------------------------------------------------------------------
void drawDisplay() {
  display.clearDisplay();

  // --- Big countdown text "mm:ss" centred on the top half ---------------
  drawBigTime(remainingSeconds, 4, 0);

  // --- Small status line -------------------------------------------------
  display.setTextSize(1);
  display.setCursor(4, 34);
  if (remainingSeconds == 0) {
    // blink the message so the end of the countdown is obvious
    if ((millis() / 500) % 2 == 0) display.print(F("TIME UP!"));
  } else if (running) {
    display.print(F("RUNNING"));           // 8 chars x 6 px = 48 px
  } else {
    display.print(F("PAUSED - press start"));  // 20 chars x 6 px = 120 px
  }

  // --- Progress bar: full width at the start, shrinks to zero -----------
  const uint8_t BAR_X = 0, BAR_Y = 46, BAR_W = 128, BAR_H = 16;
  display.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, SSD1306_WHITE);  // frame
  // inner fill: proportional to the seconds that are still left
  int16_t fillW = 0;
  if (totalSeconds > 0) {
    fillW = map(remainingSeconds, 0, totalSeconds, 0, BAR_W - 2);
  }
  if (fillW > 0) {
    display.fillRect(BAR_X + 1, BAR_Y + 1, fillW, BAR_H - 2, SSD1306_WHITE);
  }

  display.display();   // send the drawing buffer to the OLED
}

// ---------------------------------------------------------------------------
// Reset the timer for a new number of minutes
// ---------------------------------------------------------------------------
void setTime(uint8_t newMinutes) {
  minutes = newMinutes;
  totalSeconds = (long)minutes * 60;
  remainingSeconds = totalSeconds;
  running = false;
  lastTickMs = millis();
  displayDirty = true;

  Serial.print(F("Timer set to "));
  Serial.print(minutes);
  Serial.println(F(" minute(s)"));
}

// ---------------------------------------------------------------------------
// Handle all three buttons
// ---------------------------------------------------------------------------
void handleButtons() {
  if (buttonPressed(BTN_PLUS)) {
    if (minutes < MAX_MINUTES) setTime(minutes + 1);
  }
  if (buttonPressed(BTN_MINUS)) {
    if (minutes > MIN_MINUTES) setTime(minutes - 1);
  }
  if (buttonPressed(BTN_START)) {
    if (remainingSeconds == 0) {
      setTime(minutes);              // restart after TIME UP
    } else {
      running = !running;
      lastTickMs = millis();
      displayDirty = true;
      Serial.println(running ? F("Countdown STARTED") : F("Countdown PAUSED"));
    }
  }
}

// ---------------------------------------------------------------------------
// Count one second down whenever 1000 ms have passed while running
// ---------------------------------------------------------------------------
void updateCountdown() {
  if (!running) return;

  unsigned long now = millis();
  if (now - lastTickMs >= 1000) {
    lastTickMs += 1000;             // keep the ticks accurate even if loop() is slow
    if (remainingSeconds > 0) {
      remainingSeconds--;
      displayDirty = true;
      Serial.print(F("Remaining: "));
      Serial.print(remainingSeconds);
      Serial.println(F(" s"));
    }
    if (remainingSeconds == 0) {
      running = false;
      Serial.println(F("TIME UP!"));
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("042 - OLED Countdown Display starting..."));

  // Buttons are read through the internal pull-up resistor:
  // pin is HIGH when released, LOW when pressed (other side wired to GND)
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttonState[i] = HIGH;
    lastReading[i] = HIGH;
    lastDebounceMs[i] = 0;
  }

  // --- Start the OLED ----------------------------------------------------
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("ERROR: SSD1306 OLED not found!"));
    while (true) { delay(1000); }    // stop, there is nothing to show
  }
  Serial.println(F("SSD1306 OLED found."));

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Countdown timer"));
  display.println(F("MIN+ / MIN- / START"));
  display.display();
  delay(1200);

  setTime(START_MINUTES);
  Serial.println(F("Press the buttons: pin2 = minutes+, pin3 = minutes-, pin4 = start/stop"));
}

void loop() {
  handleButtons();     // react to the three push buttons
  updateCountdown();   // tick the timer once per second while it runs

  if (displayDirty) {  // redraw only when something actually changed
    drawDisplay();
    displayDirty = false;
  }
}
