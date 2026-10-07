// 084 - Wired Controlled Robot Car
// A robot car driven from a hand controller that is connected by a long
// cable. Four buttons give FORWARD, BACKWARD, LEFT and RIGHT, and the car
// stops as soon as no button is held.
// Board: Arduino Uno
//
// WHY A WIRED (TETHERED) REMOTE?
// The controller is simply four push buttons and a ground wire running down
// one long cable. There is no radio, no pairing, no battery powered
// transmitter and nothing that can drop out mid drive, so the link is 100%
// reliable and the sketch stays simple enough for a first robot project.
//
// Wiring:
//   Hand controller (each button: one leg -> pin, other leg -> GND):
//     FORWARD button -> digital pin 2
//     BACKWARD button -> digital pin 3
//     LEFT button -> digital pin 4
//     RIGHT button -> digital pin 5
//     All four share one ground wire that returns to Arduino GND, so the
//     tether needs five conductors in total (4 signals + GND).
//     The pins use INPUT_PULLUP, so no external resistors are needed.
//
//   L298N motor driver:
//     ENA -> digital pin 11 (PWM left motor speed, remove the ENA jumper cap)
//     IN1 -> digital pin 6, IN2 -> digital pin 7   (left motor)
//     IN3 -> digital pin 8, IN4 -> digital pin 9   (right motor)
//     ENB -> digital pin 10 (PWM right motor speed, remove the ENB jumper cap)
//     +12V -> separate 6-12V motor battery pack (+)
//     GND  -> battery (-) AND Arduino GND (common ground is essential)
//
// PIN NOTE: the usual robot wiring puts ENA on pin 5, but this project needs
// pins 2-5 for the four tether buttons, so ENA was moved to pin 11 (still a
// PWM pin). IN1-IN4 and ENB stay on the standard pins 6, 7, 8, 9 and 10.
//
// IMPORTANT: power the motors from a separate 6-12V supply, never from the
// Arduino 5V pin, and always join the supply ground to the Arduino ground.

// ---------------- Pin constants ----------------
const int BTN_FORWARD = 2;   // held = drive forwards
const int BTN_BACKWARD = 3;  // held = drive backwards
const int BTN_LEFT = 4;      // held = turn left
const int BTN_RIGHT = 5;     // held = turn right

const int ENA_PIN = 11;  // L298N enable A (PWM) - left motor speed (moved from 5)
const int IN1_PIN = 6;   // L298N left motor input 1
const int IN2_PIN = 7;   // L298N left motor input 2
const int IN3_PIN = 8;   // L298N right motor input 1
const int IN4_PIN = 9;   // L298N right motor input 2
const int ENB_PIN = 10;  // L298N enable B (PWM) - right motor speed

// ---------------- Behaviour settings ----------------
const int DRIVE_SPEED = 180;   // 0-255 straight line speed
const int TURN_SPEED = 170;    // 0-255 pivot speed
const int BUTTON_PRESSED = LOW;  // INPUT_PULLUP reads LOW when pressed
const unsigned long PRINT_MS = 500;  // Serial heartbeat interval

// ---------------- State variables ----------------
char lastCommand[12] = "";   // last command printed on the Serial Monitor
unsigned long lastPrintMs = 0;

// Drive the left motor. Speed is signed: + forward, - reverse, 0 stop.
void setLeftMotor(int speed) {
  if (speed > 0) {
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, speed);
  } else if (speed < 0) {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, HIGH);
    analogWrite(ENA_PIN, -speed);
  } else {
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, 0);
  }
}

// Drive the right motor in exactly the same way
void setRightMotor(int speed) {
  if (speed > 0) {
    digitalWrite(IN3_PIN, HIGH);
    digitalWrite(IN4_PIN, LOW);
    analogWrite(ENB_PIN, speed);
  } else if (speed < 0) {
    digitalWrite(IN3_PIN, LOW);
    digitalWrite(IN4_PIN, HIGH);
    analogWrite(ENB_PIN, -speed);
  } else {
    digitalWrite(IN3_PIN, LOW);
    digitalWrite(IN4_PIN, LOW);
    analogWrite(ENB_PIN, 0);
  }
}

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
}

// Spin left on the spot: left wheel back, right wheel forward
void pivotLeft(int speed) {
  setLeftMotor(-speed);
  setRightMotor(speed);
}

// Spin right on the spot: left wheel forward, right wheel back
void pivotRight(int speed) {
  setLeftMotor(speed);
  setRightMotor(-speed);
}

bool buttonHeld(int pin) {
  return digitalRead(pin) == BUTTON_PRESSED;
}

// Read all four buttons and return the winning command.
// If two buttons are held together the first one in this list wins.
const char* readCommand() {
  if (buttonHeld(BTN_FORWARD))  return "FORWARD";
  if (buttonHeld(BTN_BACKWARD)) return "BACKWARD";
  if (buttonHeld(BTN_LEFT))     return "LEFT";
  if (buttonHeld(BTN_RIGHT))    return "RIGHT";
  return "STOP";   // no button held -> coast to a stop every cycle
}

void setup() {
  // INPUT_PULLUP gives a solid HIGH when the button is open and LOW when it
  // is pressed to ground, so no external resistors are required.
  pinMode(BTN_FORWARD, INPUT_PULLUP);
  pinMode(BTN_BACKWARD, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  stopMotors();

  Serial.begin(9600);
  Serial.println("084 - Wired Controlled Robot Car ready.");
  Serial.println("Commands: FORWARD / BACKWARD / LEFT / RIGHT / STOP");
}

void loop() {
  const char* command = readCommand();

  // ----- 1. Run the command every cycle (this is what stops the car) -----
  if (strcmp(command, "FORWARD") == 0) {
    setLeftMotor(DRIVE_SPEED);
    setRightMotor(DRIVE_SPEED);
  } else if (strcmp(command, "BACKWARD") == 0) {
    setLeftMotor(-DRIVE_SPEED);
    setRightMotor(-DRIVE_SPEED);
  } else if (strcmp(command, "LEFT") == 0) {
    pivotLeft(TURN_SPEED);
  } else if (strcmp(command, "RIGHT") == 0) {
    pivotRight(TURN_SPEED);
  } else {
    stopMotors();   // button released -> stop right away
  }

  // ----- 2. Log each new command, plus a half second heartbeat -----
  if (strcmp(command, lastCommand) != 0 || millis() - lastPrintMs >= PRINT_MS) {
    strcpy(lastCommand, command);
    lastPrintMs = millis();

    Serial.print("Command: ");
    Serial.print(command);
    Serial.print(" | Buttons F:");
    Serial.print(buttonHeld(BTN_FORWARD) ? 1 : 0);
    Serial.print(" B:");
    Serial.print(buttonHeld(BTN_BACKWARD) ? 1 : 0);
    Serial.print(" L:");
    Serial.print(buttonHeld(BTN_LEFT) ? 1 : 0);
    Serial.print(" R:");
    Serial.print(buttonHeld(BTN_RIGHT) ? 1 : 0);
    Serial.println();
  }

  delay(25);
}
