// 085 - Sumo Robot
// A mini-sumo robot: it stays inside the ring, hunts for an opponent and
// charges when it sees one.
// Board: Arduino Uno
//
// SUMO RING RULES (mini-sumo / "minisumo" class):
// The robots fight inside a round ring called a dohyo. The ring floor is
// white with a black border line about 25 mm wide, and each robot must stay
// inside it. Our two downward sensors watch for that black border, which is
// the "edge" the robot must never cross. In a real bout, if neither robot
// has pushed the other for 30 seconds the referee stops the match, so a
// robot that only attacks when it actually sees an opponent saves its
// battery and avoids pointless shoving - that is exactly what this sketch
// does with its SEARCH -> ATTACK -> EDGE AVOID state machine.
//
// Wiring:
//   Left downward reflectance sensor  OUT -> digital pin 2
//   Right downward reflectance sensor OUT -> digital pin 3
//     (each module: VCC -> 5V, GND -> GND, mounted under the chassis
//      looking at the floor, about 5 mm above the ring surface)
//   HC-SR04 TRIG -> digital pin 11, ECHO -> digital pin 12, VCC -> 5V, GND -> GND
//     (facing forwards at opponent height)
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
// Priority of the three states (checked in this order every cycle):
//   1. EDGE AVOID - a border line is under a sensor: back away first, it is
//      always the most urgent action.
//   2. ATTACK     - an opponent is within 100 cm: charge at full speed.
//   3. SEARCH     - nothing seen: sweep the ring with slow spins.

// ---------------- Pin constants ----------------
const int EDGE_LEFT_PIN = 2;   // left downward reflectance sensor
const int EDGE_RIGHT_PIN = 3;  // right downward reflectance sensor

const int TRIG_PIN = 11;   // HC-SR04 trigger output
const int ECHO_PIN = 12;   // HC-SR04 echo input

const int ENA_PIN = 5;   // L298N enable A (PWM) - left motor speed
const int IN1_PIN = 6;   // L298N left motor input 1
const int IN2_PIN = 7;   // L298N left motor input 2
const int IN3_PIN = 8;   // L298N right motor input 1
const int IN4_PIN = 9;   // L298N right motor input 2
const int ENB_PIN = 10;  // L298N enable B (PWM) - right motor speed

// ---------------- Behaviour settings ----------------
const int ATTACK_SPEED = 230;    // 0-255, charging speed
const int SEARCH_SPEED = 150;    // 0-255, slower while hunting
const int EDGE_SPEED = 200;      // 0-255, escape speed at the border
const int OPPONENT_CM = 100;     // closer than this counts as an opponent
const bool EDGE_LEVEL = LOW;     // sensors read LOW over the black border
const unsigned long EDGE_MS = 800;       // escape manoeuvre length
const unsigned long SEARCH_SPIN_MS = 1500;  // sweep direction changes
const unsigned long ECHO_TIMEOUT_US = 30000;  // 30 ms ≈ max 5 m, stops pulseIn() hanging
const unsigned long PRINT_MS = 700;      // Serial heartbeat interval

// ---------------- State machine ----------------
enum RobotState { SEARCHING, ATTACKING, AVOIDING_EDGE };
RobotState state = SEARCHING;
unsigned long stateStartMs = 0;
unsigned long searchStartMs = 0;
bool searchLeft = true;      // which way the search spin is going
int lastDistance = -1;       // last opponent distance, -1 = nothing in range
unsigned long lastPrintMs = 0;

// A floor sensor reports the border line when its output matches EDGE_LEVEL
bool edgeSeen(int pin) {
  return digitalRead(pin) == EDGE_LEVEL;
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

void stopMotors() {
  setLeftMotor(0);
  setRightMotor(0);
}

// Charge straight at the opponent
void charge(int speed) {
  setLeftMotor(speed);
  setRightMotor(speed);
}

// Arc backwards away from the border. The wheel on the side that saw the
// line spins faster in reverse, so the robot also turns away from that edge.
void backAwayFromEdge(bool leftEdge) {
  if (leftEdge) {
    setLeftMotor(-EDGE_SPEED);        // left wheel fast back
    setRightMotor(-EDGE_SPEED / 2);   // right wheel slow back -> swings right
  } else {
    setLeftMotor(-EDGE_SPEED / 2);
    setRightMotor(-EDGE_SPEED);
  }
}

// Sweep the ring: left wheel back, right wheel forward (or the mirror image)
void searchSpin(int speed) {
  if (searchLeft) {
    setLeftMotor(-speed);
    setRightMotor(speed);
  } else {
    setLeftMotor(speed);
    setRightMotor(-speed);
  }
}

// Measure the distance in centimetres, or return -1 when no echo comes back
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1;      // no echo = nothing in range
  return (long)(duration / 58);      // ~58 microseconds per cm round trip
}

void enterState(RobotState next) {
  state = next;
  stateStartMs = millis();
}

// Human readable state names for the Serial log
const char* stateName(RobotState s) {
  switch (s) {
    case SEARCHING:     return "SEARCH";
    case ATTACKING:     return "ATTACK";
    case AVOIDING_EDGE: return "EDGE AVOID";
  }
  return "?";
}

void setup() {
  pinMode(EDGE_LEFT_PIN, INPUT);
  pinMode(EDGE_RIGHT_PIN, INPUT);
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
  Serial.println("085 - Sumo Robot ready.");
  Serial.println("States: SEARCH / ATTACK / EDGE AVOID");
  Serial.println("Place the robot inside the ring and press the motor supply on.");
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Inputs (read every cycle, they are cheap) -----
  bool leftEdge = edgeSeen(EDGE_LEFT_PIN);
  bool rightEdge = edgeSeen(EDGE_RIGHT_PIN);
  bool opponent = false;

  if (state != AVOIDING_EDGE) {          // do not chase anything while escaping
    lastDistance = readDistanceCm();
    opponent = (lastDistance > 0 && lastDistance <= OPPONENT_CM);
  }

  // ----- 2. Priority: escape the border, attack, otherwise search -----
  if (leftEdge || rightEdge) {
    if (state != AVOIDING_EDGE) {
      Serial.println("State: EDGE AVOID");
      enterState(AVOIDING_EDGE);
    }
    backAwayFromEdge(leftEdge);
  } else if (opponent) {
    if (state != ATTACKING) {
      Serial.println("State: ATTACK");
      enterState(ATTACKING);
    }
    charge(ATTACK_SPEED);
  } else {
    if (state != SEARCHING) {
      Serial.println("State: SEARCH");
      enterState(SEARCHING);
      searchStartMs = now;
    }

    // Flip the sweep direction every 1.5 s so the robot covers the ring
    if (now - searchStartMs >= SEARCH_SPIN_MS) {
      searchStartMs = now;
      searchLeft = !searchLeft;
      Serial.print("State: SEARCH (now spinning ");
      Serial.print(searchLeft ? "left" : "right");
      Serial.println(")");
    }
    searchSpin(SEARCH_SPEED);
  }

  // ----- 3. Leave the escape state once the robot is back inside -----
  if (state == AVOIDING_EDGE && !leftEdge && !rightEdge && now - stateStartMs >= EDGE_MS) {
    Serial.println("State: SEARCH");
    enterState(SEARCHING);
    searchStartMs = now;
  }

  // ----- 4. Heartbeat with the state and the last distance -----
  if (now - lastPrintMs >= PRINT_MS) {
    lastPrintMs = now;
    Serial.print("State: ");
    Serial.print(stateName(state));
    Serial.print(" | Opponent: ");
    if (lastDistance < 0) {
      Serial.print("none");
    } else {
      Serial.print(lastDistance);
      Serial.print(" cm");
    }
    Serial.print(" | Edge L:");
    Serial.print(leftEdge ? 1 : 0);
    Serial.print(" R:");
    Serial.println(rightEdge ? 1 : 0);
  }

  delay(20);
}
