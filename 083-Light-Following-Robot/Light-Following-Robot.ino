// 083 - Light Following Robot
// A two-wheel robot that steers towards the brightest light source using
// two LDR voltage dividers and an L298N motor driver.
// Board: Arduino Uno
//
// Wiring:
//   Left LDR divider:  5V -> LDR -> analog pin A0 -> 10k ohm -> GND
//   Right LDR divider: 5V -> LDR -> analog pin A1 -> 10k ohm -> GND
//     (more light on the LDR = higher analog reading, 0...1023)
//
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
// Steering rules:
//   both readings dark                 -> stop (nothing to follow)
//   both sides within the deadband     -> drive straight (balanced light)
//   left much brighter than right      -> pivot left  towards the light
//   right much brighter than left      -> pivot right towards the light

// ---------------- Pin constants ----------------
const int LDR_LEFT_PIN = A0;    // left LDR divider
const int LDR_RIGHT_PIN = A1;   // right LDR divider

const int ENA_PIN = 5;   // L298N enable A (PWM) - left motor speed
const int IN1_PIN = 6;   // L298N left motor input 1
const int IN2_PIN = 7;   // L298N left motor input 2
const int IN3_PIN = 8;   // L298N right motor input 1
const int IN4_PIN = 9;   // L298N right motor input 2
const int ENB_PIN = 10;  // L298N enable B (PWM) - right motor speed

// ---------------- Behaviour settings ----------------
const int CRUISE_SPEED = 170;   // 0-255 straight line speed
const int TURN_SPEED = 165;     // 0-255 pivot speed towards the light
const int DEADBAND = 60;        // L-R difference considered "balanced"
const int DARK_LEVEL = 150;     // both below this = room is too dark
const float SMOOTHING = 0.25;   // low pass filter weight for new samples (0-1)
const unsigned long PRINT_MS = 500;  // Serial heartbeat interval

// ---------------- State variables ----------------
float leftValue = 0;         // smoothed left reading
float rightValue = 0;        // smoothed right reading
char lastAction[16] = "";    // last printed decision
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

void driveForward(int speed) {
  setLeftMotor(speed);
  setRightMotor(speed);
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

// Average several analog readings so single noisy samples cannot steer us
int readSmoothed(int pin) {
  long total = 0;
  for (byte i = 0; i < 8; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return (int)(total / 8);
}

// Merge a fresh sample into the filtered value (simple low pass filter)
float smooth(float previous, int raw) {
  return previous + SMOOTHING * ((float)raw - previous);
}

void setup() {
  pinMode(LDR_LEFT_PIN, INPUT);
  pinMode(LDR_RIGHT_PIN, INPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  stopMotors();

  // Load the filters with real values so the robot starts from a true state
  leftValue = readSmoothed(LDR_LEFT_PIN);
  rightValue = readSmoothed(LDR_RIGHT_PIN);

  Serial.begin(9600);
  Serial.println("083 - Light Following Robot ready.");
  Serial.println("Shine a torch on one side and watch the robot steer to it.");
}

void loop() {
  // ----- 1. Read both dividers and update the filters -----
  int rawLeft = readSmoothed(LDR_LEFT_PIN);
  int rawRight = readSmoothed(LDR_RIGHT_PIN);
  leftValue = smooth(leftValue, rawLeft);
  rightValue = smooth(rightValue, rawRight);

  int left = (int)leftValue;
  int right = (int)rightValue;
  int diff = left - right;          // positive = left side is brighter
  int gap = abs(diff);

  // ----- 2. Choose the driving action -----
  const char* action;

  if (left < DARK_LEVEL && right < DARK_LEVEL) {
    // Nothing bright enough to follow - wait quietly
    action = "STOP (dark)";
    stopMotors();
  } else if (gap <= DEADBAND) {
    // Both sides see about the same light - go straight ahead
    action = "FORWARD";
    driveForward(CRUISE_SPEED);
  } else if (diff > 0) {
    // Left side is brighter - turn towards it
    action = "TURN LEFT";
    pivotLeft(TURN_SPEED);
  } else {
    // Right side is brighter - turn towards it
    action = "TURN RIGHT";
    pivotRight(TURN_SPEED);
  }

  // ----- 3. Report the values plus action on change and every 500 ms -----
  if (strcmp(action, lastAction) != 0 || millis() - lastPrintMs >= PRINT_MS) {
    strcpy(lastAction, action);
    lastPrintMs = millis();

    Serial.print("L:");
    Serial.print(left);
    Serial.print(" R:");
    Serial.print(right);
    Serial.print(" diff:");
    Serial.print(diff);
    Serial.print(" | Action: ");
    Serial.println(action);
  }

  delay(25);
}
