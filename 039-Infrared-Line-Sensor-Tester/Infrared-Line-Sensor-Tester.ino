// 039 - Infrared Line Sensor Tester
// Tests a TCRT5000 reflectance sensor by showing its raw analog value and
// its thresholded digital state in the Serial Monitor.
// Board: Arduino Uno
// Wiring:
//   TCRT5000 module  ->  VCC = 5V, GND = GND
//                       AO -> pin A0  (raw reflected-light value, 0..1023)
//                       DO -> pin 2   (comparator output, already thresholded)
//   Built-in LED     ->  pin 13 lights up when a line is detected
//
// IMPORTANT: every TCRT5000 module has a small potentiometer on it. Turn it to
// set the point where DO flips between HIGH and LOW. Adjust it until the
// printed LINE / NO LINE matches a white and a black surface held under the
// sensor. The raw A0 value is what you watch while tuning.

// ---- Pin constants ----
const int LINE_DO_PIN = 2;   // Digital output from the module's comparator
const int LINE_AO_PIN = A0;  // Raw analog reflection value
const int LED_PIN = 13;      // Built-in LED shows the digital detection state

// ---- Timing constants ----
const unsigned long SAMPLE_MS = 200;  // Refresh the readings 5 times per second

unsigned long lastSample = 0;
bool lastLine = false;   // Previous detection state, used to print only changes

void setup() {
  pinMode(LINE_DO_PIN, INPUT);
  pinMode(LINE_AO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Infrared Line Sensor Tester starting...");
  Serial.println("raw | state");
}

void loop() {
  // Sample at a fixed rate so the Serial output stays readable
  if (millis() - lastSample < SAMPLE_MS) {
    return;
  }
  lastSample = millis();

  // ---- Read both outputs of the module ----
  int rawValue = analogRead(LINE_AO_PIN);     // 0..1023 reflection level
  int doState = digitalRead(LINE_DO_PIN);     // Thresholded HIGH / LOW

  // On a dark surface almost no IR light is reflected, so AO drops low and
  // the module's comparator pulls DO LOW as well.
  bool lineDetected = (doState == LOW);

  // LED mirrors the detection so you can see it without a computer
  digitalWrite(LED_PIN, lineDetected ? HIGH : LOW);

  // ---- Print a full line whenever the detection state changes, and keep a
  //      running raw value so you can watch the potentiometer tuning ----
  if (lineDetected != lastLine) {
    Serial.print(lineDetected ? "LINE   " : "NO LINE");
    Serial.print("  raw=");
    Serial.println(rawValue);
    lastLine = lineDetected;
  } else {
    Serial.print("raw=");
    Serial.println(rawValue);
  }
}
