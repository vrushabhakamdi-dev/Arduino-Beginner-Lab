// 082 - Obstacle Avoiding Robot
// An HC-SR04 ultrasonic sensor watches the road ahead. When something is
// closer than 25 cm the robot stops, reverses for half a second, turns and
// then drives on, logging every decision on the Serial Monitor.
// Board: Arduino Uno
//
// Wiring:
//   HC-SR04 TRIG -> digital pin 11, ECHO -> digital pin 12, VCC -> 5V, GND -> GND
//   L298N motor driver (standard robot wiring):
//     ENA -> digital pin 5  (PWM left motor speed, remove the ENA jumper cap)
//     IN1 -> digital pin 6, IN2 -> digital pin 7   (left motor)
//     IN3 -> digital pin 8, IN4 -> digital pin 9   (right motor)
//     ENB -> digital pin 10 (PWM right motor speed, remove the ENB jumper cap)
//     +12V -> separate 6-12V motor battery pack (+)
//     GND  -> battery (-) AND Arduino GND (common ground is essential)
//
// IMPORTANT: power the motors from a separate 6-12V supply, never from the
// Arduino 5V pin, and always join the supply ground to the Arduino ground.
//
// If the robot turns the wrong way, swap the two wires of that motor.
//
// The robot runs a three phase state machine:
//   FORWARD -> obstacle closer than 25 cm -> stop (150 ms)
//            -> reverse for 500 ms -> turn for 600 ms -> FORWARD again
// The escape turn alternates left / right so the robot does not get trapped
// bouncing between the same two walls.

// ---------------- Pin constants ----------------
const int TRIG_PIN = 11;   // HC-SR04 trigger output
const int ECHO_PIN = 12;   // HC-SR04 echo input

const int ENA_PIN = 5;   // L298N enable A (PWM) - left motor speed
const int IN1_PIN = 6;   // L298N left motor input 1
const int IN2_PIN = 7;   // L298N left motor input 2
const int IN3_PIN = 8;   // L298N right motor input 1
const int IN4_PIN = 9;   // L298N right motor input 2
const int ENB_PIN = 10;  // L298N enable B (PWM) - right motor speed

// ---------------- Behaviour settings ----------------
const int CRUISE_SPEED = 170;          // 0-255 driving speed
const int OBSTACLE_CM = 25;            // closer than this = obstacle
const unsigned long STOP_MS = 150;     // brief pause before reversing
const unsigned long REVERSE_MS = 500;  // back away for 0.5 seconds
const unsigned long TURN_MS = 600;     // turn for 0.6 seconds
const unsigned long ECHO_TIMEOUT_US = 30000;  // 30 ms ≈ max 5 m, stops pulseIn() hanging
const unsigned long PRINT_MS = 500;    // Serial heartbeat interval

// ---------------- State machine ----------------
enum RobotState { FORWARD, STOPPING, REVERSING, TURNING };
RobotState state = FORWARD;
unsigned long stateStartMs = 0;   // when the current phase began
bool turnLeftNext = false;        // alternate the escape direction
int lastDistance = -1;            // last reading, -1 = out of range
unsigned long lastPrintMs = 0;

// Measure the distance in centimetres, or return -1 when no echo comes back
long readDistanceCm() {
  // Send a clean 10 microsecond trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1;      // no echo = nothing in range
  return (long)(duration / 58);      // ~58 microseconds per cm round trip
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

void driveForward(int speed) {
  setLeftMotor(speed);
  setRightMotor(speed);
}

void driveReverse(int speed) {
  setLeftMotor(-speed);
  setRightMotor(-speed);
}

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
}

// Spin on the spot. dirLeft = true turns the robot to the left.
void spin(bool dirLeft, int speed) {
  if (dirLeft) {
    setLeftMotor(-speed);   // left wheel back, right wheel forward
    setRightMotor(speed);
  } else {
    setLeftMotor(speed);
    setRightMotor(-speed);
  }
}

// Move into a new phase and restart its timer
void enterState(RobotState next) {
  state = next;
  stateStartMs = millis();
}

// Human readable name of the current phase, used for the Serial log
const char* stateName(RobotState s) {
  switch (s) {
    case FORWARD:  return "FORWARD";
    case STOPPING: return "OBSTACLE";
    case REVERSING:return "REVERSE";
    case TURNING:  return "TURN";
  }
  return "?";
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  stopMotors();

  Serial.begin(9600);
  Serial.println("082 - Obstacle Avoiding Robot ready.");
  Serial.println("Decision log: FORWARD / OBSTACLE / REVERSE / TURN");
}

void loop() {
  unsigned long now = millis();

  switch (state) {
    // ----- 1. Drive ahead and look for trouble -----
    case FORWARD:
      driveForward(CRUISE_SPEED);
      lastDistance = readDistanceCm();

      if (lastDistance > 0 && lastDistance <= OBSTACLE_CM) {
        stopMotors();
        Serial.print("OBSTACLE at ");
        Serial.print(lastDistance);
        Serial.println(" cm -> STOP");
        enterState(STOPPING);
      }
      break;

    // ----- 2. Stand still for a moment so the reverse starts cleanly -----
    case STOPPING:
      stopMotors();
      if (now - stateStartMs >= STOP_MS) {
        driveReverse(CRUISE_SPEED);
        turnLeftNext = !turnLeftNext;   // alternate left / right escapes
        Serial.println("REVERSE 500 ms");
        enterState(REVERSING);
      }
      break;

    // ----- 3. Back away from the obstacle -----
    case REVERSING:
      driveReverse(CRUISE_SPEED);
      if (now - stateStartMs >= REVERSE_MS) {
        stopMotors();
        Serial.print("TURN ");
        Serial.println(turnLeftNext ? "LEFT" : "RIGHT");
        enterState(TURNING);
      }
      break;

    // ----- 4. Turn to a new heading, then resume driving -----
    case TURNING:
      spin(turnLeftNext, CRUISE_SPEED);
      if (now - stateStartMs >= TURN_MS) {
        stopMotors();
        Serial.println("FORWARD");
        enterState(FORWARD);
      }
      break;
  }

  // ----- Heartbeat: distance plus current phase every 500 ms -----
  if (now - lastPrintMs >= PRINT_MS) {
    lastPrintMs = now;
    Serial.print("Last distance: ");
    if (lastDistance < 0) {
      Serial.print("out of range");
    } else {
      Serial.print(lastDistance);
      Serial.print(" cm");
    }
    Serial.print(" | Phase: ");
    Serial.println(stateName(state));
  }

  delay(20);
}
