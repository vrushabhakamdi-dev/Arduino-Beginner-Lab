/*
 * 096 - Automatic Railway Crossing
 * Board: Arduino Uno
 *
 * A level-crossing model that works like the real thing: when the first IR
 * sensor sees a train approaching, the servo on pin 6 lowers the barrier,
 * the two crossing LEDs on pins 4 and 5 blink alternately and the buzzer on
 * pin 8 rings like a crossing bell. When the second IR sensor sees the tail
 * of the train leaving, the barrier rises again and everything goes quiet.
 *
 * The whole thing is a small STATE MACHINE:
 *   OPEN     -> barrier up, alarms off, waiting for a train
 *   CLOSING  -> barrier is travelling down, alarms already active
 *   CLOSED   -> barrier is down, alarms active, waiting for the train to pass
 *   OPENING  -> barrier is travelling up, alarms stop as soon as it starts
 * Printing each transition to the Serial Monitor makes the state machine
 * easy to follow while it runs.
 *
 * Wiring:
 *   IR sensor "train approaching" OUT -> digital pin 2 (VCC->5V, GND->GND)
 *   IR sensor "train leaving"    OUT -> digital pin 3 (VCC->5V, GND->GND)
 *   Crossing LED 1 + 220 ohm -> digital pin 4
 *   Crossing LED 2 + 220 ohm -> digital pin 5
 *   Servo brown/black -> GND, red -> 5V, orange -> digital pin 6
 *   Passive buzzer + -> digital pin 8 (through 100 ohm), - -> GND
 *
 * Most FC-51 style IR modules output LOW when they see an object; if yours
 * behaves the other way, change IR_ACTIVE_LOW to false.
 *
 * SAFETY: educational prototype only - see the SAFETY NOTE in
 * project_info.txt. It is not real railway equipment.
 */

#include <Servo.h>   // built-in Servo library

// ---------------- Named pin constants ----------------
const int TRAIN_IN_PIN = 2;    // IR sensor: train coming towards the crossing
const int TRAIN_OUT_PIN = 3;   // IR sensor: train leaving the crossing
const int LED_A_PIN = 4;       // crossing warning light 1
const int LED_B_PIN = 5;       // crossing warning light 2
const int SERVO_PIN = 6;       // barrier servo
const int BUZZER_PIN = 8;      // crossing bell

// ---------------- Behaviour constants ----------------
const bool IR_ACTIVE_LOW = true;      // module reports LOW when it sees the train
const int BARRIER_UP_ANGLE = 0;       // barrier raised, road open
const int BARRIER_DOWN_ANGLE = 90;    // barrier lowered, road closed
const unsigned long STEP_MS = 20;     // one degree of barrier travel every 20 ms
const unsigned long CONFIRM_MS = 150; // sensor must see the train this long
const unsigned long LED_BLINK_MS = 400;   // alternating warning light speed
const unsigned long BELL_PERIOD_MS = 600; // full ring cycle
const unsigned long BELL_ON_MS = 80;      // length of one bell strike

// ---------------- State machine ----------------
enum CrossingState { STATE_OPEN, STATE_CLOSING, STATE_CLOSED, STATE_OPENING };
CrossingState currentState = STATE_OPEN;
CrossingState previousState = STATE_OPEN;

Servo barrierServo;
int barrierAngle = BARRIER_UP_ANGLE;

// Sensor confirmation bookkeeping (time the train was first seen)
unsigned long inSeenSinceMs = 0;   // 0 = not seen
unsigned long outSeenSinceMs = 0;

const char *stateName(CrossingState s) {
  switch (s) {
    case STATE_OPEN:    return "OPEN";
    case STATE_CLOSING: return "CLOSING";
    case STATE_CLOSED:  return "CLOSED";
    default:            return "OPENING";
  }
}

void changeState(CrossingState next) {
  previousState = currentState;
  currentState = next;

  // Every state starts with fresh confirmation windows, so a detection that
  // began in the previous state cannot trigger the next action by itself.
  inSeenSinceMs = 0;
  outSeenSinceMs = 0;

  Serial.print("State: ");
  Serial.print(stateName(previousState));
  Serial.print(" -> ");
  Serial.println(stateName(currentState));
}

// Confirmation check for each sensor so the timing rule is obvious.
// Returns true only after the detection has been stable for CONFIRM_MS.
bool sensorConfirms(int pin, unsigned long &seenSince) {
  int raw = digitalRead(pin);
  bool active = IR_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);

  if (!active) {
    seenSince = 0;      // object gone -> forget the pending detection
    return false;
  }
  if (seenSince == 0) {
    seenSince = millis();
    return false;       // just appeared, start the confirmation window
  }
  return (millis() - seenSince) >= CONFIRM_MS;
}

// Alternating warning lights + bell, used while the barrier is down.
void updateAlarms() {
  bool alarmsOn = (currentState == STATE_CLOSING || currentState == STATE_CLOSED);

  if (!alarmsOn) {
    digitalWrite(LED_A_PIN, LOW);
    digitalWrite(LED_B_PIN, LOW);
    noTone(BUZZER_PIN);
    return;
  }

  // Alternating flash: phase flips every LED_BLINK_MS.
  bool phase = ((millis() / LED_BLINK_MS) % 2) == 0;
  digitalWrite(LED_A_PIN, phase ? HIGH : LOW);
  digitalWrite(LED_B_PIN, phase ? LOW : HIGH);

  // Bell strike: tone for BELL_ON_MS inside every BELL_PERIOD_MS window.
  if ((millis() % BELL_PERIOD_MS) < BELL_ON_MS) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }
}

// Move the barrier one degree per STEP_MS without blocking the loop, so the
// lights and bell keep running while the barrier travels.
void updateBarrier() {
  if (currentState == STATE_CLOSING) {
    static unsigned long lastStepMs = 0;
    if (millis() - lastStepMs >= STEP_MS) {
      lastStepMs = millis();
      if (barrierAngle < BARRIER_DOWN_ANGLE) {
        barrierAngle++;
        barrierServo.write(barrierAngle);
      }
      if (barrierAngle >= BARRIER_DOWN_ANGLE) changeState(STATE_CLOSED);
    }
  } else if (currentState == STATE_OPENING) {
    static unsigned long lastStepMs = 0;
    if (millis() - lastStepMs >= STEP_MS) {
      lastStepMs = millis();
      if (barrierAngle > BARRIER_UP_ANGLE) {
        barrierAngle--;
        barrierServo.write(barrierAngle);
      }
      if (barrierAngle <= BARRIER_UP_ANGLE) {
        changeState(STATE_OPEN);
        Serial.println("Crossing clear - road open.");
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRAIN_IN_PIN, INPUT);
  pinMode(TRAIN_OUT_PIN, INPUT);
  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_A_PIN, LOW);
  digitalWrite(LED_B_PIN, LOW);
  noTone(BUZZER_PIN);

  barrierServo.attach(SERVO_PIN);
  barrierServo.write(BARRIER_UP_ANGLE);   // road starts open

  Serial.println("Automatic Railway Crossing ready.");
  Serial.println("State: OPEN - waiting for a train at the approach sensor.");
}

void loop() {
  // ---- 1. Train arriving: close the crossing ----
  if (currentState == STATE_OPEN &&
      sensorConfirms(TRAIN_IN_PIN, inSeenSinceMs)) {
    Serial.println("Train APPROACHING -> lowering barrier.");
    changeState(STATE_CLOSING);
  }

  // ---- 2. Train leaving: reopen the crossing ----
  if ((currentState == STATE_CLOSED || currentState == STATE_CLOSING) &&
      sensorConfirms(TRAIN_OUT_PIN, outSeenSinceMs)) {
    Serial.println("Train LEAVING -> raising barrier.");
    changeState(STATE_OPENING);
  }

  updateAlarms();   // warning lights + bell (only when the barrier is down)
  updateBarrier();  // one degree at a time, never blocking

  delay(5);
}
