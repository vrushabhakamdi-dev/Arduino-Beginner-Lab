// 024 - IR Remote LED Controller
// Uses a VS1838 infrared receiver to read button presses from a normal TV
// remote and toggle three LEDs with three different remote buttons.
// Board: Arduino Uno
// Wiring:
//   VS1838 receiver (looking at the lens side, pins left to right):
//     OUT/Data -> digital pin 2
//     GND      -> GND
//     VCC      -> 5V
//     Also place a 100 ohm resistor in series with VCC if your module has
//     no onboard one, and a 10 uF capacitor across VCC/GND for clean power.
//   LED 1 -> pin 4 through a 220 ohm resistor
//   LED 2 -> pin 5 through a 220 ohm resistor
//   LED 3 -> pin 6 through a 220 ohm resistor
//
// Library required: "IRremote" by shirriff (install via the Library Manager)
//   Sketch -> Include Library -> Manage Libraries... -> search "IRremote"

#include <IRremote.hpp>   // Must be included for decoding IR signals
// Note: IRremote 4.x uses IRremote.hpp. If you installed the older 3.x
// version, change this line to #include <IRremote.h> instead.

// ---------------- Pin constants ----------------
const int IR_RECEIVE_PIN = 2;   // VS1838 data output
const int LED_PIN_1 = 4;        // Controlled by the first remote button
const int LED_PIN_2 = 5;        // Controlled by the second remote button
const int LED_PIN_3 = 6;        // Controlled by the third remote button

// ---------------- Remote button codes ----------------
// Every remote sends a different hexadecimal code for each button, so these
// values will almost certainly be different for YOUR remote.
//
// HOW TO CAPTURE YOUR OWN CODES:
//   1. Upload this sketch as it is.
//   2. Open the Serial Monitor at 9600 baud.
//   3. Point your remote at the VS1838 and press any button.
//   4. A line such as "Decoded NECE: address=0x0 raw=..." appears with a hex code.
//   5. Copy those hex values into the three constants below, then re-upload.
const uint32_t BUTTON_CODE_1 = 0x45;   // Replace with your own captured code
const uint32_t BUTTON_CODE_2 = 0x46;   // Replace with your own captured code
const uint32_t BUTTON_CODE_3 = 0x47;   // Replace with your own captured code

void setup() {
  // Configure the three LED pins as outputs
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);

  // Start with every LED switched off
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);
  digitalWrite(LED_PIN_3, LOW);

  Serial.begin(9600);
  Serial.println("IR Remote LED Controller starting...");
  Serial.println("Point a remote at the receiver and press buttons.");
  Serial.println("Copy the printed hex codes into BUTTON_CODE_1/2/3.");

  // Tell the IRremote library which pin the VS1838 data line is on.
  // This must be done before IrReceiver.begin().
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {
  // checkIRrecv() returns true when a complete IR code has been received
  if (IrReceiver.decode()) {
    // receivedIRQ is the raw 32-bit value the remote transmitted
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;

    // Always print the code so new remotes can be mapped quickly
    Serial.print("Received code: 0x");
    Serial.println(code, HEX);

    // Some receivers report 0 when a code repeats; ignore those
    if (code != 0) {
      if (code == BUTTON_CODE_1) {
        // toggle() flips the pin from LOW to HIGH or HIGH to LOW,
        // so one press turns the LED on and the next press turns it off
        digitalWrite(LED_PIN_1, !digitalRead(LED_PIN_1));
        Serial.print("LED 1 -> ");
        Serial.println(digitalRead(LED_PIN_1) == HIGH ? "ON" : "OFF");
      } else if (code == BUTTON_CODE_2) {
        digitalWrite(LED_PIN_2, !digitalRead(LED_PIN_2));
        Serial.print("LED 2 -> ");
        Serial.println(digitalRead(LED_PIN_2) == HIGH ? "ON" : "OFF");
      } else if (code == BUTTON_CODE_3) {
        digitalWrite(LED_PIN_3, !digitalRead(LED_PIN_3));
        Serial.print("LED 3 -> ");
        Serial.println(digitalRead(LED_PIN_3) == HIGH ? "ON" : "OFF");
      } else {
        Serial.println("Code not assigned to any LED yet.");
      }
    }

    // Resume listening for the next signal; without this line the
    // receiver would never decode a second command
    IrReceiver.resume();
  }
}
