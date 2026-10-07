/*
 * 094 - Accident Impact Detector
 * Board: Arduino Uno
 *
 * An MPU6050 accelerometer watches for a sudden mechanical shock. A normal
 * tilt or movement keeps the acceleration close to 1 g (9.81 m/s^2), but a
 * crash or heavy knock produces a very large spike for a few milliseconds.
 * When the measured magnitude jumps more than IMPACT_THRESHOLD_MPS2 away
 * from gravity, the alarm latches: LED pin 13 lights, buzzer pin 8 sounds,
 * and a timestamped "IMPACT DETECTED" line is printed to the Serial Monitor.
 * Sending the letter r in the Serial Monitor clears the alarm.
 *
 * Wiring (the MPU6050 uses the I2C bus):
 *   MPU6050 VCC -> 5V (some boards are 3.3 V - check yours), GND -> GND
 *   MPU6050 SDA -> analog pin A4 (Arduino I2C data)
 *   MPU6050 SCL -> analog pin A5 (Arduino I2C clock)
 *   MPU6050 INT -> not used by this sketch
 *   LED + 220 ohm -> digital pin 13 (built-in LED is fine)
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *
 * Mount the sensor rigidly on the object you want to monitor (a model car,
 * a helmet, a package) - a loose sensor will not see a clean impact spike.
 *
 * Libraries (Sketch -> Include Library -> Manage Libraries):
 *   Adafruit MPU6050, Adafruit Unified Sensor, Adafruit BusIO
 *
 * SAFETY: educational prototype only - see the SAFETY NOTE in
 * project_info.txt. It is not a vehicle event recorder or airbag trigger.
 */

#include <Adafruit_MPU6050.h>   // driver for the MPU6050 accelerometer
#include <Adafruit_Sensor.h>    // Adafruit Unified Sensor interface
#include <Wire.h>               // built-in I2C library used by the driver

// ---------------- Named pin constants ----------------
const int LED_PIN = 13;      // impact indicator
const int BUZZER_PIN = 8;    // impact buzzer

// The MPU6050 is on the I2C bus: SDA -> A4, SCL -> A5 (fixed by the board).

// ---------------- Detection settings ----------------
// Gravity itself is 9.81 m/s^2, and sitting still the sensor reads about
// that. An impact is a spike far away from it. 30 m/s^2 (about 3 g of
// deviation) is a good starting point - lower it for gentler knocks,
// raise it if you see false triggers.
const float GRAVITY_MPS2 = 9.81;
const float IMPACT_THRESHOLD_MPS2 = 30.0;

const unsigned long STATUS_MS = 500;   // print the live reading twice a second

Adafruit_MPU6050 mpu;
sensors_event_t accelEvent, gyroEvent, tempEvent;

bool alarmLatched = false;   // stays true until 'r' is received on Serial
unsigned long lastStatusMs = 0;
unsigned long impactCount = 0;

// Prints a [MM:SS.mmm] timestamp so impacts can be lined up on a timeline.
void printTimestamp() {
  unsigned long ms = millis();
  unsigned long totalSeconds = ms / 1000;
  unsigned long minutes = totalSeconds / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long millisPart = ms % 1000;

  Serial.print('[');
  if (minutes < 10) Serial.print('0');
  Serial.print(minutes);
  Serial.print(':');
  if (seconds < 10) Serial.print('0');
  Serial.print(seconds);
  Serial.print('.');
  if (millisPart < 100) Serial.print('0');
  if (millisPart < 10) Serial.print('0');
  Serial.print(millisPart);
  Serial.print("] ");
}

// Clears the latched alarm after the operator types 'r'.
void handleSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      alarmLatched = false;
      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);
      printTimestamp();
      Serial.println("Alarm cleared by operator ('r').");
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.println("Accident Impact Detector starting...");

  // Start the I2C bus and talk to the MPU6050 at address 0x68.
  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found. Check SDA -> A4, SCL -> A5.");
    while (true) { delay(1000); }   // nothing else can work without the sensor
  }

  // +-8 g gives plenty of headroom for a crash spike while staying precise
  // enough for everyday movement.
  mpu.setRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 found (range +-8 g, I2C SDA=A4, SCL=A5).");
  printTimestamp();
  Serial.print("Impact threshold: ");
  Serial.print(IMPACT_THRESHOLD_MPS2, 1);
  Serial.println(" m/s^2 away from gravity. Send 'r' to clear the alarm.");
}

void loop() {
  // --- 1. Read one acceleration sample from the sensor ---
  mpu.getEvent(&accelEvent, &gyroEvent, &tempEvent);
  float ax = accelEvent.acceleration.x;
  float ay = accelEvent.acceleration.y;
  float az = accelEvent.acceleration.z;

  // Vector magnitude of the acceleration (Pythagoras in 3 axes).
  float magnitude = sqrt(ax * ax + ay * ay + az * az);

  // Distance from normal gravity - a spike means a shock or impact.
  float deviation = abs(magnitude - GRAVITY_MPS2);

  // --- 2. Latch the alarm on the first impact ---
  if (deviation > IMPACT_THRESHOLD_MPS2 && !alarmLatched) {
    alarmLatched = true;
    impactCount++;
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1800);           // alert tone until reset

    printTimestamp();
    Serial.print("IMPACT DETECTED - magnitude ");
    Serial.print(magnitude, 1);
    Serial.print(" m/s^2 (");
    Serial.print(deviation, 1);
    Serial.println(" away from gravity). Send 'r' to clear.");
  }

  // --- 3. Accept a reset from the Serial Monitor ---
  handleSerialCommands();

  // --- 4. Live reading for tuning the threshold ---
  if (millis() - lastStatusMs >= STATUS_MS) {
    lastStatusMs = millis();
    printTimestamp();
    Serial.print("magnitude=");
    Serial.print(magnitude, 2);
    Serial.print(" m/s^2 | state=");
    Serial.print(alarmLatched ? "IMPACT LATCHED" : "monitoring");
    Serial.print(" | impacts=");
    Serial.println(impactCount);
  }

  delay(10);   // about 100 samples per second, plenty for impact detection
}
