// 079 - Water Tank Level Controller
// Measure the water level from the top of a tank with an HC-SR04 sensor and
// switch a (low-voltage demo) pump through a relay: the pump starts when the
// level falls below 20% and stops when the tank reaches 95%.
// Board: Arduino Uno
//
// SENSOR PLACEMENT: the HC-SR04 is mounted ABOVE the water, looking straight
// down, fixed to the tank lid or bracket. Keep it at least 2-3 cm away from the
// wall so echoes from the wall are not read as water. The empty distance is the
// full depth of the tank; the water surface reflects the sound back.
//
// Wiring:
//   HC-SR04 TRIG -> pin 9, HC-SR04 ECHO -> pin 10, VCC -> 5V, GND -> GND
//   Relay module IN -> pin 4 (switches the pump supply)
//   LED (pump running) -> pin 13 through a 220 ohm resistor
//
// SAFETY: Educational prototype only. Demo with a low-voltage DC pump (5-12 V)
// and a relay module. Never put the Arduino, breadboard or USB cable near
// water, and never switch mains electricity with this circuit.

// ---------------- Pin constants ----------------
const int TRIG_PIN = 9;    // Ultrasonic trigger
const int ECHO_PIN = 10;   // Ultrasonic echo
const int RELAY_PIN = 4;   // Relay that switches the pump
const int LED_PIN = 13;    // LED mirrors the pump state

// ---------------- Level settings ----------------
const int TANK_DEPTH_CM = 50;      // Distance from the sensor face to the tank bottom
                                   // (include any mounting height above the tank)
const int PUMP_ON_PERCENT = 20;    // Start filling when the level is at or below 20%
const int PUMP_OFF_PERCENT = 95;   // Stop filling when the level reaches 95%
const unsigned long ECHO_TIMEOUT_US = 30000;  // 30 ms ≈ max 5 m, stops pulseIn() hanging
const unsigned long MEASURE_INTERVAL_MS = 500;  // Measure twice a second

// ---------------- Relay logic ----------------
// Most cheap relay modules are ACTIVE-LOW (coil energised when IN is LOW).
// Change this constant to false if your module switches the other way.
const bool RELAY_ACTIVE_LOW = true;

// ---------------- State variables ----------------
bool pumpOn = false;
int currentPercent = 0;
int cycleCount = 0;              // How many fill cycles have completed
unsigned long lastMeasureMs = 0;
unsigned long lastLogMs = 0;

// Convert our ON/OFF flag into the correct level for this relay module
int relayPinLevel(bool on) {
  if (RELAY_ACTIVE_LOW) return on ? LOW : HIGH;
  return on ? HIGH : LOW;
}

// Measure the distance to the water surface in cm, or -1 when there is no echo
long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1;
  return (long)(duration / 58);  // ~58 microseconds per cm (round trip)
}

// Turn the pump on or off and write an entry to the fill cycle log
void setPump(bool on, int levelPercent) {
  if (on == pumpOn) return;
  pumpOn = on;

  digitalWrite(RELAY_PIN, relayPinLevel(on));
  digitalWrite(LED_PIN, on ? HIGH : LOW);

  if (on) {
    Serial.print("PUMP ON - filling started (level ");
    Serial.print(levelPercent);
    Serial.println(" %)");
  } else {
    cycleCount++;
    Serial.print("PUMP OFF - target reached (level ");
    Serial.print(levelPercent);
    Serial.print(" %) | fill cycles completed: ");
    Serial.println(cycleCount);
  }
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Make sure the pump is off at startup
  digitalWrite(RELAY_PIN, relayPinLevel(false));
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Water Tank Level Controller ready.");
  Serial.println("Pump starts at <=20% and stops at >=95% (hysteresis).");
}

void loop() {
  unsigned long now = millis();

  // ----- 1. Take a distance measurement twice a second -----
  if (now - lastMeasureMs >= MEASURE_INTERVAL_MS) {
    lastMeasureMs = now;

    long distanceCm = readDistanceCm();

    if (distanceCm < 0 || distanceCm > TANK_DEPTH_CM + 5) {
      // No echo (tank empty / sensor misaligned): keep the previous state
      Serial.println("Warning: no valid echo - keeping the previous pump state.");
    } else {
      // The sensor looks down from the top: small distance = lots of water
      int levelPercent = map(distanceCm, 0, TANK_DEPTH_CM, 100, 0);
      if (levelPercent < 0) levelPercent = 0;
      if (levelPercent > 100) levelPercent = 100;
      currentPercent = levelPercent;

      // ----- 2. Pump control with hysteresis -----
      if (!pumpOn && currentPercent <= PUMP_ON_PERCENT) {
        setPump(true, currentPercent);
      } else if (pumpOn && currentPercent >= PUMP_OFF_PERCENT) {
        setPump(false, currentPercent);
      }

      // ----- 3. Level line while the pump is running -----
      if (pumpOn && now - lastLogMs >= 2000) {
        lastLogMs = now;
        Serial.print("Filling... level ");
        Serial.print(currentPercent);
        Serial.println(" %");
      }
    }
  }

  // ----- 4. Steady status line every second -----
  static unsigned long lastStatusMs = 0;
  if (now - lastStatusMs >= 1000) {
    lastStatusMs = now;
    Serial.print("Level: ");
    Serial.print(currentPercent);
    Serial.print(" % | Pump: ");
    Serial.print(pumpOn ? "RUNNING" : "STOPPED");
    Serial.print(" | Cycles: ");
    Serial.println(cycleCount);
  }

  delay(20);
}
