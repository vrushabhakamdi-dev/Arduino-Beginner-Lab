// 044 - LED Matrix Scrolling Message
// Scrolls the text "HELLO ARDUINO" left across an 8x8 MAX7219 LED matrix
// module, over and over again.
// Board: Arduino Uno
//
// Libraries (Arduino IDE -> Sketch -> Include Library -> Manage Libraries):
//   - MD_Parola     (by MajicDesigns / Marco Colli)
//   - MD_MAX72XX    (by MajicDesigns / Marco Colli)  <- installed as a dependency
//
// Wiring:
//   MAX7219 module DIN  -> Arduino pin 11 (MOSI)
//   MAX7219 module CLK  -> Arduino pin 13 (SCK)
//   MAX7219 module CS   -> Arduino pin 10 (chip select)
//   MAX7219 module VCC  -> 5V, GND -> GND

#include <SPI.h>          // the MAX7219 is driven over the SPI bus
#include <MD_MAX72xx.h>   // low level driver for the MAX7219 chip
#include <MD_Parola.h>    // text effects and scrolling engine on top of it

// ---------------------------------------------------------------------------
// Hardware constants
// ---------------------------------------------------------------------------
// If your matrix shows garbled or rotated characters, try another module type,
// e.g. MD_MAX72XX::FC16_HW, MD_MAX72XX::GENERIC_HW or MD_MAX72XX::PAROLA_HW.
const MD_MAX72XX::moduleType_t HARDWARE_TYPE = MD_MAX72XX::FC16_HW;

const uint8_t MAX_DEVICES = 4;   // number of 8x8 modules in the chain (1 to 8)
const uint8_t DATA_PIN = 11;     // SPI MOSI  - module DIN
const uint8_t CLK_PIN = 13;      // SPI SCK   - module CLK
const uint8_t CS_PIN = 10;       // chip select - module CS

// ---------------------------------------------------------------------------
// Display settings - change these to personalise the project
// ---------------------------------------------------------------------------
const char MESSAGE[] = "HELLO ARDUINO";  // <-- put your own text here (max ~100 chars)

const uint8_t BRIGHTNESS = 7;      // intensity from 0 (dim) to 15 (bright)
const uint16_t SCROLL_SPEED = 40;  // milliseconds per animation frame;
                                   // LOWER = faster, e.g. 15 = quick, 80 = slow
const textPosition_t TEXT_ALIGN = PA_LEFT;   // where the text starts
const textEffect_t TEXT_EFFECT = PA_SCROLL_LEFT;  // scroll direction

// ---------------------------------------------------------------------------
// The Parola display object: module type, DATA, CLK, CS pins, device count
// ---------------------------------------------------------------------------
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void setup() {
  Serial.begin(9600);
  Serial.println(F("044 - LED Matrix Scrolling Message starting..."));

  // Start the matrix (begin() returns false if the hardware is not answering)
  if (!matrix.begin()) {
    Serial.println(F("ERROR: MAX7219 matrix not found – check DIN/CLK/CS!"));
    while (true) { delay(1000); }
  }

  matrix.setIntensity(BRIGHTNESS);        // 0..15 brightness
  matrix.setCharSpacing(2);               // 2 blank columns between letters
  matrix.setSpeed(SCROLL_SPEED);          // animation frame time

  // Load the message and start the left scrolling animation.
  // The library keeps the text inside a RAM buffer, so MESSAGE must stay
  // in memory (it is a global const char[], that is fine).
  matrix.displayScroll(MESSAGE, TEXT_ALIGN, TEXT_EFFECT, SCROLL_SPEED);

  Serial.print(F("Scrolling: "));
  Serial.println(MESSAGE);
}

void loop() {
  // displayAnimate() advances the animation by one frame.
  // It returns TRUE when the whole message has finished one pass.
  if (matrix.displayAnimate()) {
    // Animation finished -> restart it so the message loops forever.
    matrix.displayReset();
    Serial.println(F("Message pass complete, restarting..."));
  }
}
