// 049 - Joystick Servo Controller
// Two small servos follow the joystick: X controls one arm, Y controls the
// other. Pressing the joystick button centres both servos at 90 degrees.
// Board: Arduino Uno (uses the Servo library included with the Arduino IDE)
//
// Wiring:
//   Joystick module -> 5V, GND
//        VRx (X)  -> A0
//        VRy (Y)  -> A1
//        SW (push)-> pin 2 to GND (internal pull-up is used)
//   Servo 1 signal wire -> pin 9   (its red wire to 5V, brown/black to GND)
//   Servo 2 signal wire -> pin 10

#include <Servo.h>   // built-in Arduino library, no download needed

// ---------------------------------------------------------------------------
// Pin constants
// ---------------------------------------------------------------------------
const uint8_t JOYSTICK_X_PIN = A0;    // horizontal axis, 0..1023
const uint8_t JOYSTICK_Y_PIN = A1;    // vertical axis, 0..1023
const uint8_t JOYSTICK_SW_PIN = 2;    // joystick push button

const uint8_t SERVO_X_PIN = 9;        // servo moved by the X axis
const uint8_t SERVO_Y_PIN = 10;       // servo moved by the Y axis

// ---------------------------------------------------------------------------
// Behaviour constants
// ---------------------------------------------------------------------------
const int CENTER_ANGLE = 90;          // middle position of a standard servo
const int MIN_ANGLE = 0;
const int MAX_ANGLE = 180;
const uint8_t SMOOTH_SAMPLES = 8;     // samples averaged per reading
const int DEAD_ZONE = 20;             // degrees near the centre that stay put
const unsigned long DEBOUNCE_MS = 30;
const unsigned long SERIAL_MS = 400;  // print the angles about twice a second

// ---------------------------------------------------------------------------
// Global objects and state
// ---------------------------------------------------------------------------
Servo servoX;   // servo driven by the joystick X axis
Servo servoY;   // servo driven by the joystick Y axis

int angleX = CENTER_ANGLE;            // current angle sent to servo X
int angleY = CENTER_ANGLE;            // current angle sent to servo Y

// Smoothing filter state: the filtered value is kept between loop() calls so
// the movement glides instead of jumping. An alternative one-liner would be an
// exponential filter: smooth = smooth + (raw - smooth) / 4;
int smoothX = 512;
int smoothY = 512;

int buttonState = HIGH;               // debounced state, LOW = pressed
int lastReading = HIGH;
unsigned long lastDebounceMs = 0;
unsigned long lastSerialMs = 0;

// ---------------------------------------------------------------------------
// Average several analogue samples to remove electrical noise.
// ---------------------------------------------------------------------------
int readSmoothed(uint8_t pin) {
  long total = 0;
  for (uint8_t i = 0; i < SMOOTH_SAMPLES; i++) {
    total += analogRead(pin);
    delayMicroseconds(150);
  }
  return (int)(total / SMOOTH_SAMPLES);   // 0..1023, much steadier than one read
}

// ---------------------------------------------------------------------------
// Convert an axis reading (0..1023) into a servo angle (0..180).
// Values near the centre are pulled to the centre so the servo does not jitter.
// ---------------------------------------------------------------------------
int angleFromAxis(int value) {
  int angle = map(value, 0, 1023, MIN_ANGLE, MAX_ANGLE);
  if (abs(angle - CENTER_ANGLE) < DEAD_ZONE) {
    angle = CENTER_ANGLE;   // inside the dead zone: hold the centre position
  }
  return constrain(angle, MIN_ANGLE, MAX_ANGLE);
}

// ---------------------------------------------------------------------------
// Debounced button check, returns true once per press
// ---------------------------------------------------------------------------
bool buttonPressed() {
  bool newlyPressed = false;
  int reading = digitalRead(JOYSTICK_SW_PIN);

  if (reading != lastReading) lastDebounceMs = millis();
  lastReading = reading;

  if ((millis() - lastDebounceMs) >= DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) newlyPressed = true;
    }
  }
  return newlyPressed;
}

void printAngles() {
  Serial.print(F("Servo X = "));
  Serial.print(angleX);
  Serial.print(F(" deg   Servo Y = "));
  Serial.print(angleY);
  Serial.println(F(" deg"));
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("049 - Joystick Servo Controller starting..."));

  servoX.attach(SERVO_X_PIN);   // start controlling the servo on pin 9
  servoY.attach(SERVO_Y_PIN);   // start controlling the servo on pin 10

  // Start centred so both arms begin in the middle
  servoX.write(CENTER_ANGLE);
  servoY.write(CENTER_ANGLE);
  delay(300);

  pinMode(JOYSTICK_SW_PIN, INPUT_PULLUP);
  buttonState = digitalRead(JOYSTICK_SW_PIN);
  lastReading = buttonState;

  Serial.println(F("Move the joystick to steer, press it to re-centre."));
  printAngles();
}

void loop() {
  // --- Smoothed joystick readings ----------------------------------------
  smoothX = readSmoothed(JOYSTICK_X_PIN);
  smoothY = readSmoothed(JOYSTICK_Y_PIN);

  // --- Button: centre both servos immediately ----------------------------
  if (buttonPressed()) {
    angleX = CENTER_ANGLE;
    angleY = CENTER_ANGLE;
    servoX.write(angleX);
    servoY.write(angleY);
    Serial.println(F("Recentred both servos"));
    printAngles();
    lastSerialMs = millis();
  } else {
    // --- Follow the joystick ---------------------------------------------
    int newX = angleFromAxis(smoothX);
    int newY = angleFromAxis(smoothY);

    // Only move a servo when the angle really changed; this keeps the PWM
    // signal calm and avoids constant tiny corrections.
    if (newX != angleX) {
      angleX = newX;
      servoX.write(angleX);
    }
    if (newY != angleY) {
      angleY = newY;
      servoY.write(angleY);
    }
  }

  // --- Periodic Serial report --------------------------------------------
  if (millis() - lastSerialMs >= SERIAL_MS) {
    lastSerialMs = millis();
    Serial.print(F("X raw = "));
    Serial.print(smoothX);
    Serial.print(F("  Y raw = "));
    Serial.print(smoothY);
    Serial.print(F("  |  "));
    printAngles();
  }
}
