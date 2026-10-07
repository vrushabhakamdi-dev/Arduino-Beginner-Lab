// 080 - Automatic Hand Sanitizer Dispenser
// Hold your hand in front of an IR obstacle sensor and a small DC pump runs
// for 1200 ms to squirt one dose of sanitizer from a reservoir.
// Board: Arduino Uno
//
// PHYSICAL BUILD: the pump sits inside (or next to) a sanitizer reservoir with
// its inlet tube in the liquid and its outlet above the nozzle. Keep the
// reservoir, tubing and any liquid WELL AWAY from the Arduino, the breadboard
// and the USB cable - moisture near electronics can cause short circuits.
//
// Wiring:
//   FC-51 IR obstacle sensor: OUT -> pin 2, VCC -> 5V, GND -> GND
//   Pump driver: pin 6 -> 1k ohm resistor -> base of an NPN transistor
//     (2N2222 or TIP120) or the gate of a logic-level NMOS
//   Pump: +V supply -> pump -> transistor collector, transistor emitter -> GND
//   Flyback diode (1N4007) across the pump, cathode to +V
//   LED -> pin 13 through a 220 ohm resistor (dispense indicator)
//
// Note: power the pump from its own supply (e.g. 4 x AA or a 5 V adapter),
// not from the Arduino 5V pin, and always share a common ground.

// ---------------- Pin constants ----------------
const int SENSOR_PIN = 2;  // FC-51 obstacle sensor output
const int PUMP_PIN = 6;    // Transistor/mosfet base or gate that drives the pump
const int LED_PIN = 13;    // "Pump running" indicator

// ---------------- Behaviour constants ----------------
// Most FC-51 modules pull the OUT pin LOW when something is in front of them.
// Change this constant if your board reports the opposite way.
const bool OBSTACLE_ACTIVE_LOW = true;

const unsigned long DISPENSE_MS = 1200;   // One dose = pump runs for 1.2 seconds
const unsigned long SENSOR_CONFIRM_MS = 80;  // Hand must be seen this long (noise filter)

// ---------------- State variables ----------------
bool pumpRunning = false;
bool handConfirmed = false;      // Debounced "hand is in front" flag
bool handWasSeen = false;        // Latched until the next dispense finishes
unsigned long handSinceMs = 0;   // When the hand first appeared (0 = not seen)
unsigned long pumpStartMs = 0;   // When the pump started
int dispenseCount = 0;           // Total doses given

// Read the sensor and convert it to "hand present"
bool handPresent() {
  int raw = digitalRead(SENSOR_PIN);
  return OBSTACLE_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

void startDispense() {
  pumpRunning = true;
  pumpStartMs = millis();
  dispenseCount++;

  digitalWrite(PUMP_PIN, HIGH);  // Turn the transistor on -> pump runs
  digitalWrite(LED_PIN, HIGH);   // Indicator LED on

  Serial.print("Dispensing dose ");
  Serial.print(dispenseCount);
  Serial.println(" - pump ON for 1200 ms.");
}

void stopDispense() {
  pumpRunning = false;
  digitalWrite(PUMP_PIN, LOW);   // Transistor off -> pump stops
  digitalWrite(LED_PIN, LOW);

  Serial.println("Dose complete - pump OFF. Remove your hand to re-arm.");
}

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);   // Make sure the pump is off at startup
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Automatic Hand Sanitizer Dispenser ready.");
  Serial.println("Place your hand in front of the IR sensor to get one dose.");
}

void loop() {
  unsigned long now = millis();
  bool hand = handPresent();

  // ----- 1. Confirm the hand with an 80 ms noise filter -----
  if (hand && !handConfirmed) {
    if (handSinceMs == 0) {
      handSinceMs = now;
    } else if (now - handSinceMs >= SENSOR_CONFIRM_MS) {
      handConfirmed = true;
      Serial.println("Hand detected.");
    }
  } else if (!hand) {
    handSinceMs = 0;
    handConfirmed = false;
    handWasSeen = false;  // The hand has left - re-arm for the next dose
  }

  // ----- 2. Start one dose -----
  // Conditions: a hand is confirmed, the pump is idle and the hand was removed
  // after the previous dose (this stops the pump from running non stop).
  if (handConfirmed && !pumpRunning && !handWasSeen) {
    startDispense();
  }

  // ----- 3. Stop the pump after exactly 1200 ms -----
  if (pumpRunning && now - pumpStartMs >= DISPENSE_MS) {
    stopDispense();
    handWasSeen = true;  // Wait for the hand to leave before the next dose
  }

  // ----- 4. Status line every second -----
  static unsigned long lastPrintMs = 0;
  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.print("Sensor: ");
    Serial.print(hand ? "hand" : "clear");
    Serial.print(" | Pump: ");
    Serial.print(pumpRunning ? "RUNNING" : "stopped");
    Serial.print(" | Doses given: ");
    Serial.println(dispenseCount);
  }

  delay(10);
}
