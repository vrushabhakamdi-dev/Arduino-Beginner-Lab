// 038 - Rotary Encoder Position Meter
// Turns a KY-040 rotary encoder into a counter that goes up and down as you spin it.
// Board: Arduino Uno
// Wiring (KY-040 module):
//   CLK -> pin 2
//   DT  -> pin 3
//   SW  -> pin 4   (the built-in push button, uses the internal pull-up)
//   +   -> 5V
//   GND -> GND
//
// No external library is required: everything is done with plain digitalRead()
// polling. The CLK and DT signals are quadrature outputs, meaning they are
// square waves that are a quarter of a cycle out of phase. By remembering the
// previous CLK state and looking at DT when CLK changes, we can tell which way
// the knob was turned.

// ---- Pin constants ----
const int ENC_CLK_PIN = 2;   // Clock channel of the encoder
const int ENC_DT_PIN = 3;    // Data channel of the encoder
const int ENC_SW_PIN = 4;    // Push button on top of the knob (active LOW)

// ---- Timing constants ----
const unsigned long STEP_MS = 3;         // Encoder edges are fast: short gap only
const unsigned long BUTTON_DEBOUNCE_MS = 20;  // The push button bounces more
const unsigned long REPORT_MS = 100;     // Minimum gap between Serial updates

int position = 0;                // The value we are metering (can be negative)
int lastClkState = HIGH;         // CLK level at the previous loop pass
int lastSwState = HIGH;          // Button level at the previous loop pass

unsigned long lastStepTime = 0;  // millis() of the last counted step
unsigned long lastButtonTime = 0;
unsigned long lastReportTime = 0;

void setup() {
  pinMode(ENC_CLK_PIN, INPUT_PULLUP);   // Internal pull-ups keep the lines HIGH
  pinMode(ENC_DT_PIN, INPUT_PULLUP);
  pinMode(ENC_SW_PIN, INPUT_PULLUP);

  // Remember the starting level of CLK so the first change is detected correctly
  lastClkState = digitalRead(ENC_CLK_PIN);

  Serial.begin(9600);
  Serial.println("Rotary Encoder Position Meter starting...");
  Serial.println("Turn the knob to count, press it to reset to 0.");
}

void loop() {
  int clkState = digitalRead(ENC_CLK_PIN);
  int dtState = digitalRead(ENC_DT_PIN);

  // ---- Direction detection ----
  // A change on CLK marks one detent (click) of the knob.
  // If DT is LOW at that moment we turned clockwise, otherwise counter-clockwise.
  // (Swap the two branches if your encoder counts the wrong way round.)
  if (clkState != lastClkState && (millis() - lastStepTime) >= STEP_MS) {
    lastStepTime = millis();

    if (dtState != clkState) {
      position++;                          // Clockwise
      Serial.print("CW   -> position = ");
      Serial.println(position);
    } else {
      position--;                          // Counter-clockwise
      Serial.print("CCW  -> position = ");
      Serial.println(position);
    }
  }
  lastClkState = clkState;   // Store the level ready for the next pass

  // ---- Push button: resets the meter back to zero ----
  int swState = digitalRead(ENC_SW_PIN);
  if (swState == LOW && lastSwState == HIGH && (millis() - lastButtonTime) >= BUTTON_DEBOUNCE_MS) {
    lastButtonTime = millis();
    position = 0;
    Serial.println("BUTTON pressed -> position reset to 0");
  }
  lastSwState = swState;

  // ---- Periodic status line so the monitor still shows something when idle ----
  if (millis() - lastReportTime >= REPORT_MS) {
    lastReportTime = millis();
    Serial.print("position = ");
    Serial.print(position);
    Serial.print("   CLK=");
    Serial.print(clkState);
    Serial.print(" DT=");
    Serial.print(dtState);
    Serial.print(" SW=");
    Serial.println(swState == LOW ? "pressed" : "released");
  }
}
