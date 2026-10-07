// 019 - LED Heart Animation
// Board: Arduino Uno
// Wiring: 10 LEDs on pins 2-11, each through a 220 ohm resistor to GND.
//
// Physical layout - a heart shape:
//
//    pin2   pin3    -    pin4   pin5      <- top row (two lobes with a dip)
//    pin6   pin7   pin8  pin9   pin10     <- middle row
//     -      -    pin11    -      -       <- bottom tip
//
// The animation is stored as an array of frames. Every frame is one row of
// 10 numbers (1 = LED on, 0 = LED off) listed in pin order, pin 2 first.

const int LED_PIN_START = 2;   // First LED of the heart
const int LED_COUNT = 10;      // LEDs on pins 2,3,4,5,6,7,8,9,10,11

// ---------------- HOW TO EDIT THE ANIMATION ----------------
// Change the 0s and 1s below to design your own frames (always 10 values per
// frame) and change the matching value in FRAME_HOLD_MS to set how long that
// frame stays lit. A 1 lights an LED, a 0 switches it off.
const byte HEART_FRAMES[][LED_COUNT] = {
  // pin:   2  3  4  5  6  7  8  9 10 11
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // frame 0: heart at rest, all LEDs off
  {0, 0, 0, 0, 1, 1, 1, 1, 1, 0},  // frame 1: middle row fills in
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},  // frame 2: top lobes light up too
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  // frame 3: the whole heart is lit
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  // frame 4: hold the full heart (the "beat")
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // frame 5: first beat ends
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  // frame 6: quick second beat (lub-dub)
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // frame 7: second beat ends
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  // frame 8: rest before the next beat
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}   // frame 9: rest
};

// How long each frame is shown, in milliseconds (same order as HEART_FRAMES)
const unsigned int FRAME_HOLD_MS[] = {
  400, 120, 120, 140, 260, 110, 130, 110, 250, 350
};

// The number of frames is worked out from the array itself,
// so frames can be added or removed without changing this line.
const int FRAME_COUNT = sizeof(HEART_FRAMES) / sizeof(HEART_FRAMES[0]);

// Switches every LED to match one frame of the animation
void showFrame(const byte frame[]) {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PIN_START + i, frame[i] == 1 ? HIGH : LOW);
  }
}

void setup() {
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PIN_START + i, OUTPUT);
    digitalWrite(LED_PIN_START + i, LOW);
  }

  Serial.begin(9600);
  Serial.println("=== LED Heart Animation ===");
  Serial.print("Playing ");
  Serial.print(FRAME_COUNT);
  Serial.println(" frames in a loop. Edit HEART_FRAMES to change the animation.");
}

void loop() {
  for (int f = 0; f < FRAME_COUNT; f++) {
    showFrame(HEART_FRAMES[f]);

    Serial.print("Frame ");
    Serial.print(f);
    Serial.print(" held for ");
    Serial.print(FRAME_HOLD_MS[f]);
    Serial.println(" ms");

    delay(FRAME_HOLD_MS[f]);  // Keep the frame on screen for its hold time
  }
}
