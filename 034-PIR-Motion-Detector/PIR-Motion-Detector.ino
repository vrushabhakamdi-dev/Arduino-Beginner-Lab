// 034 - PIR Motion Detector
// Detects movement with an HC-SR501 PIR sensor and reports it over Serial.
// Board: Arduino Uno
// Wiring:
//   HC-SR501  ->  VCC = 5V, GND = GND, OUT = pin 2
//   Built-in LED on pin 13 lights up while motion is detected
//
// IMPORTANT SENSOR SETTINGS (see the two potentiometers on the module):
//   * Warm-up: the PIR needs about 30-60 seconds after power-up before it is
//     reliable. Do not judge the code during that first minute!
//   * Sensitivity pot (left): turn clockwise for longer detection distance.
//   * Time-delay pot (right): sets how long OUT stays HIGH after motion stops.
//   * The jumper on the back selects NON-RETRIGGERING (H) or RE-TRIGGERING (L).
//     Use the L position so OUT stays HIGH while movement continues.

// ---- Pin constants ----
const int PIR_PIN = 2;   // Digital output of the HC-SR501 (HIGH = motion)
const int LED_PIN = 13;  // Built-in LED shows the detection state

// ---- Timing constants ----
const unsigned long WARMUP_MS = 45000UL;   // 45 s warm-up message threshold
const unsigned long HEARTBEAT_MS = 1000;   // Idle status line once per second

unsigned long lastHeartbeat = 0;  // millis() of the last idle status line
unsigned long motionStart = 0;    // When the current motion period began
int motionEvents = 0;             // Number of separate motion periods seen
bool warmupDone = false;          // Have we passed the warm-up time?
bool motionActive = false;        // Is motion happening right now?

void setup() {
  pinMode(PIR_PIN, INPUT);   // The sensor drives this pin
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("PIR Motion Detector starting...");
  Serial.println("Please wait 30-60 seconds for the PIR to warm up...");
}

void loop() {
  unsigned long now = millis();

  // ---- Warm-up notice: printed once, 45 seconds after startup ----
  if (!warmupDone && now >= WARMUP_MS) {
    warmupDone = true;
    Serial.print("[");
    Serial.print(now / 1000);
    Serial.println(" s] Warm-up finished - sensor is now reliable.");
  }

  int motion = digitalRead(PIR_PIN);

  // ---- Rising edge: motion has just started ----
  if (motion == HIGH && !motionActive) {
    motionActive = true;
    motionStart = now;
    motionEvents++;

    digitalWrite(LED_PIN, HIGH);   // Visual indicator

    Serial.print("[");
    Serial.print(now / 1000);
    Serial.println(" s] MOTION DETECTED");
  }

  // ---- Falling edge: the sensor has gone quiet again ----
  if (motion == LOW && motionActive) {
    motionActive = false;
    digitalWrite(LED_PIN, LOW);

    Serial.print("[");
    Serial.print(now / 1000);
    Serial.print(" s] Motion ended after ");
    Serial.print(now - motionStart);
    Serial.println(" ms");
  }

  // ---- Idle heartbeat: proves the loop is running and shows the pin state ----
  if (now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    Serial.print("[");
    Serial.print(now / 1000);
    Serial.print(" s] status=");
    Serial.print(motionActive ? "MOTION" : "clear");
    Serial.print("  events=");
    Serial.println(motionEvents);
  }
}
