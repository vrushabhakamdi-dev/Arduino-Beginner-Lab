// 021 - Ultrasonic Distance Meter
// Measures how far an object is from an HC-SR04 ultrasonic sensor and
// prints the distance in centimetres (2 decimal places) to the Serial Monitor.
// Board: Arduino Uno
// Wiring:
//   HC-SR04 VCC  -> 5V
//   HC-SR04 GND  -> GND
//   HC-SR04 Trig -> digital pin 9
//   HC-SR04 Echo -> digital pin 10

// Pin constants: named constants instead of magic numbers
const int TRIG_PIN = 9;    // Trigger pin: we send the ultrasound pulse out here
const int ECHO_PIN = 10;   // Echo pin: the sensor returns the reflected pulse here

// Timing constant: print a new reading every 200 milliseconds
const unsigned long PRINT_INTERVAL_MS = 200;

// Speed of sound conversion: sound travels ~343 m/s = 0.0343 cm per microsecond.
// The pulse travels to the object and back, so we divide the round trip by 2.
const float MICROSECONDS_TO_CM = 0.0343 / 2.0;

// Practical limits of the HC-SR04 (anything outside these is treated as invalid)
const float MIN_DISTANCE_CM = 2.0;
const float MAX_DISTANCE_CM = 400.0;

// millis()-based timing so loop() never has to block for long
unsigned long lastPrintTime = 0;

void setup() {
  // The trigger pin is an output because we command the sensor with it
  pinMode(TRIG_PIN, OUTPUT);
  // The echo pin is an input because the sensor reports the pulse timing on it
  pinMode(ECHO_PIN, INPUT);

  // The trigger must rest LOW before each measurement
  digitalWrite(TRIG_PIN, LOW);

  // Start serial communication at 9600 baud so we can read the distances
  Serial.begin(9600);
  Serial.println("Ultrasonic Distance Meter starting...");
  Serial.println("Distance (cm):");
}

void loop() {
  // Send the 10 microsecond trigger pulse that makes the HC-SR04 fire.
  // The datasheet requires a HIGH pulse of at least 10 us.
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // pulseIn() waits for the echo pin to go HIGH, then returns how many
  // microseconds it stayed HIGH. That time is the sound's round trip.
  // The 30000 us timeout (30 ms) stops the program hanging if no echo returns.
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // Only print every PRINT_INTERVAL_MS so the Serial Monitor stays readable
  if (millis() - lastPrintTime >= PRINT_INTERVAL_MS) {
    lastPrintTime = millis();

    if (duration == 0) {
      // No echo came back in the timeout window: nothing in range, or sensor not wired
      Serial.println("Out of range (no echo)");
    } else {
      // Convert the echo time into a distance in centimetres
      float distanceCm = duration * MICROSECONDS_TO_CM;

      if (distanceCm < MIN_DISTANCE_CM || distanceCm > MAX_DISTANCE_CM) {
        // Outside the reliable window of the HC-SR04
        Serial.println("Out of range");
      } else {
        // print(value, decimals) gives exactly 2 digits after the point
        Serial.print("Distance: ");
        Serial.print(distanceCm, 2);
        Serial.println(" cm");
      }
    }
  }
}
