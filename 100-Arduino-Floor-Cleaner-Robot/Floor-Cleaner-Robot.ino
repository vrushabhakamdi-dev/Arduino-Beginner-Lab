/*
 * 100 - Arduino Floor Cleaner Robot
 * Board: Arduino Uno
 *
 * A small robot that cleans the floor by driving around on its own. Press
 * the button on pin 2 to start or stop it. While it is cleaning the robot
 * drives forward until the HC-SR04 ultrasonic sensor sees an obstacle
 * closer than OBSTACLE_CM; it then stops, picks a RANDOM left or right
 * turn, spins in place for TURN_MS and drives forward again. The brush/fan
 * motor on pin 3 runs through a transistor whenever the robot is cleaning.
 *
 * Random-bounce navigation is a classic strategy for cleaning robots:
 * there is no map, the robot simply keeps changing direction whenever it
 * hits something, and over time it covers the whole area.
 *
 * Wiring (L298N dual H-bridge):
 *   ENA -> pin 5  (PWM speed for the left motor)
 *   IN1 -> pin 6, IN2 -> pin 7   (left motor direction)
 *   IN3 -> pin 8, IN4 -> pin 9   (right motor direction)
 *   ENB -> pin 10 (PWM speed for the right motor)
 *   L298N +12V <- external battery pack (motor supply)
 *   L298N GND  <- battery GND and Arduino GND (grounds MUST be common)
 *   L298N OUT1/OUT2 -> left DC motor, OUT3/OUT4 -> right DC motor
 *
 *   HC-SR04 VCC -> 5V, GND -> GND
 *   HC-SR04 Trig -> pin 11, Echo -> pin 12
 *
 *   Start/stop button -> pin 2 -> GND (internal pull-up used)
 *   Brush/fan motor: pin 3 -> 1k ohm resistor -> base of an NPN transistor
 *     (2N2222) or logic-level NMOS; motor +V -> motor -> collector,
 *     emitter -> GND; flyback diode (1N4007) across the motor.
 *
 * MOTOR POWER: run the drive motors and the brush motor from the battery
 * pack through the L298N / transistor, never from the Arduino 5V pin. The
 * transistor exists because the motor draws far more current than a pin can
 * supply. Always connect the battery GND to the Arduino GND.
 *
 * Serial Monitor (9600) prints a navigation log: start/stop, measured
 * distance, chosen turn direction and state changes.
 */

// ---------------- L298N motor driver pins ----------------
const int ENA_PIN = 5;    // PWM speed, left motor
const int IN1_PIN = 6;    // left motor direction
const int IN2_PIN = 7;
const int IN3_PIN = 8;    // right motor direction
const int IN4_PIN = 9;
const int ENB_PIN = 10;   // PWM speed, right motor

// ---------------- Other pins ----------------
const int TRIG_PIN = 11;         // HC-SR04 trigger
const int ECHO_PIN = 12;         // HC-SR04 echo
const int START_BUTTON_PIN = 2;  // start / stop the cleaning run
const int BRUSH_MOTOR_PIN = 3;   // brush/fan motor via transistor (PWM)

// ---------------- Behaviour constants ----------------
const int OBSTACLE_CM = 20;            // closer than this = obstacle
const unsigned long TURN_MS = 500;     // length of one spot turn
const int DRIVE_SPEED = 200;           // 0-255, leaves headroom for turns
const int BRUSH_SPEED = 255;           // brush/fan runs at full speed
const unsigned long DEBOUNCE_MS = 40;  // button stability time
const unsigned long STATUS_MS = 1000;  // status line every second
const unsigned long ECHO_TIMEOUT_US = 30000;  // ~5 m - treats missing echo as "far"

// ---------------- State ----------------
bool cleaning = false;        // robot run active?
bool turning = false;         // currently spinning away from an obstacle
bool turnLeftNext = false;    // direction of the turn in progress
unsigned long turnStartMs = 0;
unsigned long lastStatusMs = 0;
unsigned long obstacleCount = 0;

int lastRawButton = HIGH;
int lastStableButton = HIGH;
unsigned long lastButtonChangeMs = 0;

// Drives one motor through the L298N.
// speed: positive = forward, negative = backward, 0 = brake.
void setMotor(int inAPin, int inBPin, int enablePin, int speed) {
  if (speed > 0) {
    digitalWrite(inAPin, HIGH);
    digitalWrite(inBPin, LOW);
    analogWrite(enablePin, speed);
  } else if (speed < 0) {
    digitalWrite(inAPin, LOW);
    digitalWrite(inBPin, HIGH);
    analogWrite(enablePin, -speed);
  } else {
    digitalWrite(inAPin, LOW);
    digitalWrite(inBPin, LOW);
    analogWrite(enablePin, 0);   // both pins low = motor stopped
  }
}

void driveForward() {
  setMotor(IN1_PIN, IN2_PIN, ENA_PIN, DRIVE_SPEED);
  setMotor(IN3_PIN, IN4_PIN, ENB_PIN, DRIVE_SPEED);
  Serial.println("Nav: driving FORWARD");
}

void spinLeft() {
  setMotor(IN1_PIN, IN2_PIN, ENA_PIN, -DRIVE_SPEED);  // left wheel back
  setMotor(IN3_PIN, IN4_PIN, ENB_PIN, DRIVE_SPEED);   // right wheel fwd
  Serial.println("Nav: turning LEFT on the spot");
}

void spinRight() {
  setMotor(IN1_PIN, IN2_PIN, ENA_PIN, DRIVE_SPEED);   // left wheel fwd
  setMotor(IN3_PIN, IN4_PIN, ENB_PIN, -DRIVE_SPEED);  // right wheel back
  Serial.println("Nav: turning RIGHT on the spot");
}

void stopMotors() {
  setMotor(IN1_PIN, IN2_PIN, ENA_PIN, 0);
  setMotor(IN3_PIN, IN4_PIN, ENB_PIN, 0);
}

// Triggers the HC-SR04 and converts the echo time to centimetres.
// Returns 999 when no echo comes back (very far away or sensor idle).
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return 999;      // timeout - treat as "nothing there"
  return duration / 58;               // speed of sound: 58 us per cm round trip
}

// Debounced start/stop button: true once per clean press.
bool startButtonPressed() {
  int raw = digitalRead(START_BUTTON_PIN);
  unsigned long now = millis();
  if (raw != lastRawButton) {
    lastRawButton = raw;
    lastButtonChangeMs = now;
  }
  if (now - lastButtonChangeMs >= DEBOUNCE_MS && raw != lastStableButton) {
    lastStableButton = raw;
    if (raw == LOW) return true;
  }
  return false;
}

void startCleaning() {
  cleaning = true;
  turning = false;
  analogWrite(BRUSH_MOTOR_PIN, BRUSH_SPEED);   // brush/fan on
  driveForward();
  Serial.println("=== CLEANING STARTED (button) ===");
}

void stopCleaning() {
  cleaning = false;
  turning = false;
  stopMotors();
  analogWrite(BRUSH_MOTOR_PIN, 0);             // brush/fan off
  Serial.println("=== CLEANING STOPPED (button) ===");
}

void setup() {
  Serial.begin(9600);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(BRUSH_MOTOR_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);

  stopMotors();
  analogWrite(BRUSH_MOTOR_PIN, 0);

  // Seed the random generator with the noise on the floating analog pin A0
  // so every power-up gives a different mix of left and right turns.
  randomSeed(analogRead(A0));

  Serial.println("Arduino Floor Cleaner Robot ready.");
  Serial.println("Press the button on pin 2 to START or STOP cleaning.");
}

void loop() {
  unsigned long now = millis();

  // ---- 1. Start / stop button ----
  if (startButtonPressed()) {
    if (cleaning) stopCleaning();
    else startCleaning();
  }

  // ---- 2. While cleaning: navigation ----
  if (cleaning) {
    if (turning) {
      // Spin for TURN_MS, then go forward again.
      if (now - turnStartMs >= TURN_MS) {
        turning = false;
        driveForward();
      }
    } else {
      long distance = readDistanceCm();

      if (distance <= OBSTACLE_CM) {
        // Obstacle ahead: stop, pick a direction at random and spin.
        obstacleCount++;
        stopMotors();
        turning = true;
        turnStartMs = now;
        turnLeftNext = (random(2) == 0);   // 0 = left, 1 = right (50/50)

        Serial.print("Obstacle at ");
        Serial.print(distance);
        Serial.print(" cm -> choosing ");
        Serial.println(turnLeftNext ? "LEFT" : "RIGHT");

        if (turnLeftNext) spinLeft();
        else spinRight();
      }
    }
  }

  // ---- 3. Status line every second ----
  if (now - lastStatusMs >= STATUS_MS) {
    lastStatusMs = now;
    if (cleaning) {
      Serial.print("Status: ");
      Serial.print(turning ? "turning" : "forward");
      Serial.print(" | distance: ");
      Serial.print(readDistanceCm());
      Serial.print(" cm | obstacles avoided: ");
      Serial.println(obstacleCount);
    } else {
      Serial.println("Status: STOPPED - press the button on pin 2 to start.");
    }
  }

  delay(20);
}
