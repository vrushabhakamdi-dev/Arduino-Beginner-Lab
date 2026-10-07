// 071 - Relay Controlled Lamp
// Press a button to toggle a relay module that switches a LOW-VOLTAGE demo lamp on and off.
// Board: Arduino Uno
// Wiring:
//   Push button: pin 2 -> GND (INPUT_PULLUP is used, no external resistor needed)
//   Relay module IN: pin 4
//   Relay module VCC: 5V, relay module GND: GND
//   LED (mirrors relay state): pin 13 through 220 ohm resistor (or the Uno's built-in LED)
//
// SAFETY: Demo only. Switch a low-voltage lamp (for example a 5V/12V bulb or LED strip).
// Never wire mains electricity to a breadboard project.

// ---------------- Pin constants ----------------
const int BUTTON_PIN = 2;   // Push button (other side to GND)
const int RELAY_PIN = 4;    // Relay module input pin
const int LED_PIN = 13;     // Built-in LED mirrors the relay state

// ---------------- Relay logic ----------------
// Most cheap relay modules are ACTIVE-LOW: the coil energises when the IN pin is LOW.
// Some modules are ACTIVE-HIGH: the coil energises when the IN pin is HIGH.
// Keep this one constant and every on/off line below works with either module.
const bool RELAY_ACTIVE_LOW = true;

// ---------------- Timing constants ----------------
const unsigned long DEBOUNCE_MS = 50;  // Ignore button bounces shorter than 50 ms

// ---------------- State variables ----------------
bool relayOn = false;             // Current lamp state
int lastButtonReading = HIGH;     // Raw reading from the last loop pass
int stableButtonState = HIGH;     // Debounced button state
unsigned long lastDebounceMs = 0; // Time of the last raw change

// Convert our own ON/OFF flag into the correct pin level for this relay module
int relayPinLevel(bool on) {
  if (RELAY_ACTIVE_LOW) {
    return on ? LOW : HIGH;  // active-low: LOW energises the coil
  } else {
    return on ? HIGH : LOW;  // active-high: HIGH energises the coil
  }
}

// Turn the relay (and the lamp) on or off
void setRelay(bool on) {
  relayOn = on;
  digitalWrite(RELAY_PIN, relayPinLevel(on));
  digitalWrite(LED_PIN, on ? HIGH : LOW);  // LED mirrors the relay state

  // Serial Monitor feedback
  if (on) {
    Serial.println("RELAY ON");
  } else {
    Serial.println("RELAY OFF");
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Internal pull-up, button reads HIGH when open
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Start serial first so the initial relay state can be printed
  Serial.begin(9600);
  Serial.println("Relay Controlled Lamp ready - press the button to toggle the lamp.");

  // Make sure the lamp starts in the OFF state
  setRelay(false);
}

void loop() {
  // ----- 1. Read and debounce the button -----
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceMs = millis();  // Reset the debounce timer on every change
  }
  lastButtonReading = reading;

  if ((millis() - lastDebounceMs) > DEBOUNCE_MS) {
    // The reading has been stable long enough, accept it
    if (reading != stableButtonState) {
      stableButtonState = reading;

      // Falling edge = button pressed (INPUT_PULLUP goes LOW when connected to GND)
      if (stableButtonState == LOW) {
        setRelay(!relayOn);  // Toggle the lamp
      }
    }
  }

  // ----- 2. Give the processor a little rest (optional but tidy) -----
  delay(10);
}
