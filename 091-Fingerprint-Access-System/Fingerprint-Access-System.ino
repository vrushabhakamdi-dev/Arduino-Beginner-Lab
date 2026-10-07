/*
 * 091 - Fingerprint Access System
 * Board: Arduino Uno
 *
 * An R503 (or R303) optical fingerprint sensor decides who may open a door.
 * A stored fingerprint that matches one of the enrolled IDs (1, 2 or 3)
 * unlocks the servo door for 4 seconds and lights the green LED. Any finger
 * that is not enrolled is rejected with the red LED and a low buzzer tone.
 *
 * ENROLLMENT (do this once, before using the sketch):
 *   1. Install the library: Sketch -> Include Library -> Manage Libraries ->
 *      search "Adafruit Fingerprint Sensor Library" and install it.
 *   2. Wire the sensor as shown below.
 *   3. Open File -> Examples -> Adafruit Fingerprint -> enroll
 *   4. Upload it, open the Serial Monitor at 9600 baud and follow the
 *      prompts to store a finger. Repeat for IDs 1, 2 and 3.
 *   5. Upload this sketch - it only compares against those stored IDs.
 *
 * Wiring:
 *   Fingerprint sensor VCC -> 5V, GND -> GND
 *   Fingerprint sensor TX  -> digital pin 2 (Arduino RX, SoftwareSerial)
 *   Arduino pin 3 (TX)     -> 1k ohm resistor -> sensor RX
 *                             2k ohm resistor -> GND   (voltage divider)
 *     The Arduino sends 5 V logic but the sensor wants about 3.3 V, so the
 *     1k/2k divider drops the 5 V from pin 3 down to roughly 3.3 V. The
 *     sensor TX -> pin 2 line needs no divider because 3.3 V is read as HIGH
 *     by the 5 V Arduino.
 *   Servo brown/black -> GND, red -> 5V (use a separate 5 V supply or the
 *     external power jack for a big servo), orange -> digital pin 6
 *   Green LED + 220 ohm resistor -> digital pin 4 (access granted)
 *   Red LED   + 220 ohm resistor -> digital pin 5 (access denied)
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 */

#include <Adafruit_Fingerprint.h>  // Adafruit Fingerprint Sensor Library
#include <SoftwareSerial.h>        // built-in: second serial port for the sensor
#include <Servo.h>                 // built-in: drives the door servo

// ---------------- Named pin constants ----------------
const int FP_RX_PIN = 2;     // Arduino receives sensor data on pin 2
const int FP_TX_PIN = 3;     // Arduino transmits to sensor on pin 3 (via divider)
const int SERVO_PIN = 6;     // servo that swings the door
const int GREEN_LED_PIN = 4; // "access granted" indicator
const int RED_LED_PIN = 5;   // "access denied" indicator
const int BUZZER_PIN = 8;    // passive buzzer feedback

// ---------------- Behaviour constants ----------------
const int FP_BAUD = 57600;       // default baud rate of the R503/R303
const int DOOR_LOCKED_ANGLE = 0;  // servo angle with the door shut
const int DOOR_OPEN_ANGLE = 90;   // servo angle with the door open
const unsigned long DOOR_OPEN_MS = 4000;  // keep the door open 4 seconds
const unsigned long DENY_BUZZ_MS = 1500;  // how long the deny tone plays

// The enrolled fingerprint IDs this sketch accepts (enroll them first).
const int ALLOWED_ID_1 = 1;
const int ALLOWED_ID_2 = 2;
const int ALLOWED_ID_3 = 3;

// SoftwareSerial uses pins 2 (RX) and 3 (TX) instead of the USB port so the
// Serial Monitor stays free for messages.
SoftwareSerial fingerSerial(FP_RX_PIN, FP_TX_PIN);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

Servo doorServo;
int grantCount = 0;   // how many times the door has been opened
int denyCount = 0;    // how many times access has been refused
int lastMatchedId = 0; // ID of the finger that was seen last (0 = none)

// Reads one fingerprint image and asks the sensor for a match.
// Returns: >= 1  -> matching enrolled ID (1, 2 or 3)
//          -1    -> no finger on the sensor
//          -2    -> sensor communication/read error
//          -3    -> finger seen, but it matches no stored template
int readFingerprint() {
  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1;
  if (p != FINGERPRINT_OK) return -2;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_NOTFOUND) {
    lastMatchedId = 0;
    return -3;
  }
  if (p != FINGERPRINT_OK) return -2;

  lastMatchedId = (int)finger.fingerID;   // which stored ID matched
  return lastMatchedId;
}

// Green LED + two short beeps + swing the door open, then close it again.
void grantAccess() {
  grantCount++;
  Serial.print("ACCESS GRANTED for finger ID ");
  Serial.println(lastMatchedId);

  digitalWrite(GREEN_LED_PIN, HIGH);
  tone(BUZZER_PIN, 1500);
  delay(120);
  noTone(BUZZER_PIN);
  delay(80);
  tone(BUZZER_PIN, 2000);
  delay(120);
  noTone(BUZZER_PIN);

  doorServo.write(DOOR_OPEN_ANGLE);   // unlock the door
  Serial.println("Door is OPEN for 4 seconds.");
  delay(DOOR_OPEN_MS);

  doorServo.write(DOOR_LOCKED_ANGLE); // lock the door again
  digitalWrite(GREEN_LED_PIN, LOW);
  Serial.println("Door LOCKED. Present a finger...");
}

// Red LED + low buzzing tone, door stays shut.
void denyAccess() {
  denyCount++;
  Serial.print("ACCESS DENIED - finger matches no enrolled ID. ");
  Serial.print("Denied total: ");
  Serial.println(denyCount);

  digitalWrite(RED_LED_PIN, HIGH);
  tone(BUZZER_PIN, 300);            // unpleasant low buzz
  delay(DENY_BUZZ_MS);
  noTone(BUZZER_PIN);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.println("Present a finger...");
}

void setup() {
  Serial.begin(9600);
  fingerSerial.begin(FP_BAUD);   // sensor side of the link

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  doorServo.attach(SERVO_PIN);
  doorServo.write(DOOR_LOCKED_ANGLE);   // always start locked

  finger.begin(FP_BAUD);

  // The sensor replies to a password query on start-up; if it does not, the
  // wiring or the power supply is wrong.
  if (!finger.verifyPassword()) {
    Serial.println("ERROR: fingerprint sensor not found.");
    Serial.println("Check wiring: sensor TX -> pin 2, pin 3 -> divider -> RX.");
    while (true) { delay(1000); }   // stop here, nothing else can work
  }

  finger.getTemplateCount();
  Serial.print("Fingerprint Access System ready. Sensor holds ");
  Serial.print(finger.templateCount);
  Serial.println(" stored template(s).");
  Serial.println("Enroll with File -> Examples -> Adafruit Fingerprint -> enroll");
  Serial.println("Accepted IDs: 1, 2 and 3. Present a finger...");
}

void loop() {
  int id = readFingerprint();

  if (id >= 1) {
    // Only IDs 1-3 are allowed; anything else is treated as a stranger.
    if (id == ALLOWED_ID_1 || id == ALLOWED_ID_2 || id == ALLOWED_ID_3) {
      grantAccess();
    } else {
      denyAccess();
    }
  } else if (id == -3) {
    denyAccess();     // a real finger, but not an enrolled one
  } else if (id == -2) {
    Serial.println("Sensor read error - try again.");
    delay(500);
  }

  // -1 (no finger) simply falls through: keep waiting quietly.

  // Periodic status line so the Serial Monitor shows the system is alive.
  static unsigned long lastStatusMs = 0;
  if (millis() - lastStatusMs >= 3000) {
    lastStatusMs = millis();
    Serial.print("Granted: ");
    Serial.print(grantCount);
    Serial.print(" | Denied: ");
    Serial.print(denyCount);
    Serial.println(" | Waiting for a finger...");
  }

  delay(50);   // poll the sensor 20 times per second
}
