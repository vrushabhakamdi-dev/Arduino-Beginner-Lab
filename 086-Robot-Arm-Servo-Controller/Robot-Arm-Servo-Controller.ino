// 086 - Robot Arm Servo Controller
// A four joint robot arm whose base, shoulder, elbow and gripper follow four
// potentiometers. The joints glide smoothly to their target instead of
// jumping, and every angle is printed on the Serial Monitor.
// Board: Arduino Uno
//
// Wiring:
//   SG90 micro servos:
//     Base     signal -> digital pin 9
//     Shoulder signal -> digital pin 10
//     Elbow    signal -> digital pin 11
//     Gripper  signal -> digital pin 12
//     (each servo: red -> 5V, brown -> GND, orange -> signal pin)
//   Potentiometers (each: one outer leg -> 5V, other outer leg -> GND,
//   wiper -> analog input):
//     Base pot      wiper -> A0
//     Shoulder pot  wiper -> A1
//     Elbow pot     wiper -> A2
//     Gripper pot   wiper -> A3
//
// POWER NOTE: four servos can stall the USB port. For reliable operation
// feed the servo 5V rail from a separate 5V supply (or a good 5V adapter)
// and join that supply's ground to the Arduino ground. Never power servos
// from the Arduino's VIN pin.
//
// JOYSTICK ALTERNATIVE: instead of four potentiometers you can use two
// joystick modules - the X axis of joystick 1 drives the base, its Y axis
// the shoulder, and joystick 2 controls elbow and gripper. The analog
// readings are identical (0-1023), so only the wiring changes, no code edit
// is needed.
//
// Libraries: Servo (built-in with the Arduino IDE)

#include <Servo.h>  // Built-in Arduino library for RC servo motors

// ---------------- Pin constants ----------------
const int SERVO_PINS[4] = {9, 10, 11, 12};        // base, shoulder, elbow, gripper
const int POT_PINS[4] = {A0, A1, A2, A3};         // matching control knobs

// ---------------- Behaviour settings ----------------
const int STEP_DEGREES = 1;        // largest move per update (smooth glide)
const unsigned long MOVE_MS = 20;  // 50 servo updates per second
const unsigned long PRINT_MS = 500;  // Serial heartbeat interval
const float POT_SMOOTHING = 0.20;  // low pass filter weight (0-1), smaller = calmer
const int START_ANGLE[4] = {90, 90, 90, 20};  // pose the arm powers up in
const char* JOINT_NAMES[4] = {"Base", "Shoulder", "Elbow", "Gripper"};

// ---------------- State variables ----------------
Servo armServo[4];
int currentAngle[4] = {90, 90, 90, 20};  // where each joint is right now
int targetAngle[4] = {90, 90, 90, 20};   // where the knob is asking for
float filteredPot[4] = {512, 512, 512, 512};  // smoothed pot readings
unsigned long lastMoveMs = 0;
unsigned long lastPrintMs = 0;

// Average a few analog samples so electrical noise cannot jitter the arm
int readPotSmoothed(int pin) {
  long total = 0;
  for (byte i = 0; i < 4; i++) {
    total += analogRead(pin);
    delay(1);
  }
  return (int)(total / 4);
}

// Move one joint exactly one step towards its target and write it out
void stepTowards(byte index) {
  if (currentAngle[index] < targetAngle[index]) {
    currentAngle[index]++;
  } else if (currentAngle[index] > targetAngle[index]) {
    currentAngle[index]--;
  }
  armServo[index].write(currentAngle[index]);
}

void setup() {
  Serial.begin(9600);
  Serial.println("086 - Robot Arm Servo Controller ready.");
  Serial.println("Turn the four potentiometers to move the arm.");

  for (byte i = 0; i < 4; i++) {
    pinMode(POT_PINS[i], INPUT);
    armServo[i].attach(SERVO_PINS[i]);

    // Start from a known pose and load the filter with a real reading
    currentAngle[i] = START_ANGLE[i];
    targetAngle[i] = START_ANGLE[i];
    armServo[i].write(currentAngle[i]);
    filteredPot[i] = readPotSmoothed(POT_PINS[i]);
  }
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Read the knobs and refresh the targets -----
  if (now - lastMoveMs >= MOVE_MS) {
    lastMoveMs = now;

    for (byte i = 0; i < 4; i++) {
      int raw = readPotSmoothed(POT_PINS[i]);

      // Low pass filter: the target only creeps towards the raw reading,
      // which removes the last trace of jitter from the potentiometer.
      filteredPot[i] = filteredPot[i] + POT_SMOOTHING * ((float)raw - filteredPot[i]);

      // Full knob travel (0-1023) maps onto the full servo sweep (0-180)
      targetAngle[i] = map((int)filteredPot[i], 0, 1023, 0, 180);

      // One degree per update = a smooth glide instead of a jump
      stepTowards(i);
    }
  }

  // ----- 2. Print the joint angles every 500 ms -----
  if (now - lastPrintMs >= PRINT_MS) {
    lastPrintMs = now;

    for (byte i = 0; i < 4; i++) {
      Serial.print(JOINT_NAMES[i]);
      Serial.print(": ");
      Serial.print(currentAngle[i]);
      if (currentAngle[i] != targetAngle[i]) {
        Serial.print("->");      // show where the joint is heading
        Serial.print(targetAngle[i]);
      }
      Serial.print("  ");
    }
    Serial.println();
  }

  delay(5);
}
