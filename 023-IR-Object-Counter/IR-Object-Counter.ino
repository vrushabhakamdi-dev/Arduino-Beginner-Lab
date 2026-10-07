// 023 - IR Object Counter
// Counts objects that pass in front of an FC-51 infrared obstacle sensor.
// Board: Arduino Uno
// Wiring:
//   FC-51 module VCC -> 5V
//   FC-51 module GND -> GND
//   FC-51 module DOUT -> digital pin 2
//
// How the module behaves:
//   The FC-51 has an IR LED and a receiver. When an object reflects the IR
//   beam back, the module's DOUT pin changes state. Many FC-51 boards are
//   ACTIVE-LOW (DOUT goes LOW when an object is detected). If your board is
//   the opposite way round, swap LOW and HIGH in the DETECTED_LEVEL constant.

// Pin constant: the sensor's digital output
const int IR_SENSOR_PIN = 2;

// Level the DOUT pin shows while an object is in front of the sensor.
// Set to LOW for the common active-low FC-51, change to HIGH if needed.
const int DETECTED_LEVEL = LOW;

// Simple debounce: ignore any new edge that happens within this many
// milliseconds of the previous one, so one object cannot be counted twice.
const unsigned long DEBOUNCE_MS = 150;

// The counter itself, printed to the Serial Monitor
int objectCount = 0;

// Bookkeeping for edge detection and debouncing
int lastLevel = HIGH;                 // Level the pin had at the previous loop
unsigned long lastEdgeTime = 0;       // millis() of the last accepted edge

void setup() {
  // The sensor output is read by the Arduino, so it is an input
  pinMode(IR_SENSOR_PIN, INPUT);

  // Start serial communication so we can see the running total
  Serial.begin(9600);
  Serial.println("IR Object Counter starting...");
  Serial.println("Type 'r' in the Serial Monitor to reset the count.");

  // Record the starting state so the first object creates a clean edge
  lastLevel = digitalRead(IR_SENSOR_PIN);

  // Print the initial value right away
  Serial.println("OBJECT COUNT: 0");
}

void loop() {
  // Read the current sensor level
  int currentLevel = digitalRead(IR_SENSOR_PIN);

  // A "falling edge" means the pin just moved from not-detected to detected.
  // That transition happens exactly once per object that passes the sensor.
  bool objectArrived = (lastLevel != DETECTED_LEVEL) && (currentLevel == DETECTED_LEVEL);

  if (objectArrived && (millis() - lastEdgeTime >= DEBOUNCE_MS)) {
    objectCount++;                       // One new object passed by
    lastEdgeTime = millis();             // Restart the debounce window

    Serial.print("OBJECT COUNT: ");
    Serial.println(objectCount);
  }

  // Remember today's level so tomorrow's loop() can spot the next change
  lastLevel = currentLevel;

  // Reset the counter when the user types 'r' in the Serial Monitor
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'r' || command == 'R') {
      objectCount = 0;
      lastEdgeTime = millis();  // Discard any edge already in flight
      Serial.println("OBJECT COUNT: 0 (counter reset)");
    }
  }
}
