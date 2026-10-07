// 081 - Line Following Robot
// A two-wheel robot that follows a black line drawn on the floor using three
// reflectance sensors and an L298N motor driver.
// Board: Arduino Uno
//
// Wiring:
//   TCRT5000 digital reflectance sensors (VCC -> 5V, GND -> GND on each):
//     LEFT   sensor OUT -> digital pin 2
//     CENTER sensor OUT -> digital pin 3
//     RIGHT  sensor OUT -> digital pin 4
//
//   L298N motor driver (standard robot wiring):
//     ENA -> digital pin 5  (PWM left motor speed, remove the ENA jumper cap)
//     IN1 -> digital pin 6, IN2 -> digital pin 7   (left motor)
//     IN3 -> digital pin 8, IN4 -> digital pin 9   (right motor)
//     ENB -> digital pin 10 (PWM right motor speed, remove the ENB jumper cap)
//     +12V -> separate 6-12V motor battery pack (+)
//     GND  -> battery (-) AND Arduino GND (common ground is essential)
//
// IMPORTANT: run the motors from a separate 6-12V supply, never from the
// Arduino 5V pin. Always join the supply ground to the Arduino ground.
//
// If the robot turns the wrong way, simply swap the two wires of that motor -
// no code change is needed.
//
// Logic (black line on a light floor, sensors read LOW over the line):
//   centre sensor on the line -> drive straight forward
//   line only on the left     -> pivot left  (robot had drifted right)
//   line only on the right    -> pivot right (robot had drifted left)
//   line completely lost      -> spin in the last direction it was seen

// ---------------- Pin constants ----------------
const int SENSOR_LEFT_PIN = 2;    // left reflectance sensor
const int SENSOR_CENTER_PIN = 3;  // centre reflectance sensor
const int SENSOR_RIGHT_PIN = 4;   // right reflectance sensor

const int ENA_PIN = 5;   // L298N enable A (PWM) - left motor speed
const int IN1_PIN = 6;   // L298N left motor input 1
const int IN2_PIN = 7;   // L298N left motor input 2
const int IN3_PIN = 8;   // L298N right motor input 1
const int IN4_PIN = 9;   // L298N right motor input 2
const int ENB_PIN = 10;  // L298N enable B (PWM) - right motor speed

// ---------------- Behaviour settings ----------------
const int CRUISE_SPEED = 170;    // 0-255, speed while driving straight
const int TURN_SPEED = 160;      // 0-255, pivot speed while correcting
const int SEARCH_SPEED = 140;    // 0-255, slower spin while searching
const bool LINE_LEVEL = LOW;     // most TCRT5000 modules output LOW over black
const unsigned long PRINT_MS = 1000;  // Serial heartbeat interval

// ---------------- State variables ----------------
enum SearchDirection { SEARCH_LEFT, SEARCH_RIGHT };
SearchDirection lastSeenSide = SEARCH_RIGHT;  // where the line was last seen
char lastAction[16] = "";                    // last printed decision
unsigned long lastPrintMs = 0;               // heartbeat timer

// A sensor reports the line when its output matches LINE_LEVEL
bool lineSeen(int pin) {
  return digitalRead(pin) == LINE_LEVEL;
}

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

// Both wheels forward
void driveForward(int speed) {
  setLeftMotor(speed);
  setRightMotor(speed);
}

// Both wheels stopped
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

// Print the raw sensor pattern together with the chosen action
void printStatus(bool leftOn, bool centerOn, bool rightOn, const char* action) {
  Serial.print("Sensors L:");
  Serial.print(leftOn ? 1 : 0);
  Serial.print(" C:");
  Serial.print(centerOn ? 1 : 0);
  Serial.print(" R:");
  Serial.print(rightOn ? 1 : 0);
  Serial.print(" | Action: ");
  Serial.println(action);
}

void setup() {
  pinMode(SENSOR_LEFT_PIN, INPUT);
  pinMode(SENSOR_CENTER_PIN, INPUT);
  pinMode(SENSOR_RIGHT_PIN, INPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  stopMotors();  // never start with the wheels spinning

  Serial.begin(9600);
  Serial.println("081 - Line Following Robot ready.");
  Serial.println("Put the robot on a black line and watch the sensor pattern.");
}

void loop() {
  // ----- 1. Read the three sensors once per cycle -----
  bool leftOn = lineSeen(SENSOR_LEFT_PIN);
  bool centerOn = lineSeen(SENSOR_CENTER_PIN);
  bool rightOn = lineSeen(SENSOR_RIGHT_PIN);

  // ----- 2. Choose the driving action -----
  const char* action;

  if (centerOn) {
    // The line is under the middle sensor - drive straight ahead
    action = "FORWARD";
    driveForward(CRUISE_SPEED);
  } else if (leftOn && rightOn) {
    // Both side sensors see the line (wide line or crossing) - keep going
    action = "FORWARD";
    driveForward(CRUISE_SPEED);
  } else if (leftOn) {
    // Line drifted to the left sensor - turn left to get back on it
    action = "TURN LEFT";
    lastSeenSide = SEARCH_LEFT;
    pivotLeft(TURN_SPEED);
  } else if (rightOn) {
    // Line drifted to the right sensor - turn right to get back on it
    action = "TURN RIGHT";
    lastSeenSide = SEARCH_RIGHT;
    pivotRight(TURN_SPEED);
  } else {
    // Line lost completely - search by spinning the way we last saw it
    if (lastSeenSide == SEARCH_LEFT) {
      action = "SEARCH LEFT";
      pivotLeft(SEARCH_SPEED);
    } else {
      action = "SEARCH RIGHT";
      pivotRight(SEARCH_SPEED);
    }
  }

  // ----- 3. Report every change of decision plus a 1 second heartbeat -----
  if (strcmp(action, lastAction) != 0 || millis() - lastPrintMs >= PRINT_MS) {
    strcpy(lastAction, action);
    lastPrintMs = millis();
    printStatus(leftOn, centerOn, rightOn, action);
  }

  delay(20);  // short pause keeps the loop stable at about 50 reads/second
}
