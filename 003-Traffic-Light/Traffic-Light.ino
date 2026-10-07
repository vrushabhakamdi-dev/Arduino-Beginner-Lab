// 003 - Traffic Light
// A simple three-LED traffic light that cycles Red -> Yellow -> Green
// with realistic timing, and reports the current light in the Serial Monitor.
// Board: Arduino Uno
// Wiring: Red LED -> pin 2, Yellow LED -> pin 3, Green LED -> pin 4,
//         each through a 220 ohm resistor to GND.

// Pin constants for the three lights
const int RED_PIN = 2;
const int YELLOW_PIN = 3;
const int GREEN_PIN = 4;

// Timing constants in milliseconds (1000 ms = 1 second)
const unsigned long RED_TIME_MS = 3000;     // Red stays on for 3 seconds
const unsigned long YELLOW_TIME_MS = 2000;  // Yellow stays on for 2 seconds
const unsigned long GREEN_TIME_MS = 5000;   // Green stays on for 5 seconds

void setup() {
  // Set all three light pins as outputs
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Traffic Light starting...");
}

// Helper: turn on exactly one light and turn the other two off
void setLight(int onPin) {
  digitalWrite(RED_PIN, onPin == RED_PIN ? HIGH : LOW);
  digitalWrite(YELLOW_PIN, onPin == YELLOW_PIN ? HIGH : LOW);
  digitalWrite(GREEN_PIN, onPin == GREEN_PIN ? HIGH : LOW);
}

void loop() {
  // Red phase: stop traffic
  setLight(RED_PIN);
  Serial.println("Light state: RED (stop)");
  delay(RED_TIME_MS);

  // Yellow phase: get ready to go
  setLight(YELLOW_PIN);
  Serial.println("Light state: YELLOW (caution)");
  delay(YELLOW_TIME_MS);

  // Green phase: go
  setLight(GREEN_PIN);
  Serial.println("Light state: GREEN (go)");
  delay(GREEN_TIME_MS);

  // loop() restarts, so the light cycles forever
}
