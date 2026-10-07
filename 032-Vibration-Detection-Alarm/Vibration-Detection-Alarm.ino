// 032 - Vibration Detection Alarm
// Detects a knock, bump or vibration with an SW-420 sensor and raises an alarm.
// Board: Arduino Uno
// Wiring:
//   SW-420 vibration sensor  ->  VCC = 5V, GND = GND, DO = pin 2
//   Buzzer (+)               ->  pin 8 through a 220 ohm resistor, buzzer (-) = GND
//   LED anode (+)            ->  pin 13 (built-in LED on the Uno, no resistor needed)
//
// The module's on-board potentiometer sets how sensitive it is.

// ---- Pin constants ----
const int VIBRATION_PIN = 2;  // Digital output of the SW-420 module (HIGH = vibration)
const int BUZZER_PIN = 8;     // Piezo buzzer that sounds the alarm
const int LED_PIN = 13;       // Built-in indicator LED

// ---- Timing constants ----
const unsigned long DEBOUNCE_MS = 300;  // Ignore repeat triggers for 300 ms

unsigned long lastEventTime = 0;   // millis() of the last accepted event
unsigned long lastPrintTime = 0;   // Used only to keep the Serial log tidy
int eventCount = 0;                // How many vibration events we have seen

void setup() {
  pinMode(VIBRATION_PIN, INPUT);   // The sensor drives this pin
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Start serial communication so we can log each event
  Serial.begin(9600);
  Serial.println("Vibration Detection Alarm starting...");
  Serial.println("Knock the sensor to trigger the alarm.");
}

void loop() {
  int vibrationState = digitalRead(VIBRATION_PIN);

  // ---- Debounce: a single knock can make the pin chatter HIGH/LOW many times.
  //      We only accept a NEW event if DEBOUNCE_MS has passed since the last one.
  // ----
  if (vibrationState == HIGH && (millis() - lastEventTime) >= DEBOUNCE_MS) {
    lastEventTime = millis();   // Remember when this event happened
    eventCount++;

    // Alarm outputs
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1500);      // 1.5 kHz alarm tone

    // Log the event with a millisecond timestamp so you can see the spacing
    Serial.print("[");
    Serial.print(lastEventTime);
    Serial.print(" ms] VIBRATION DETECTED #");
    Serial.println(eventCount);

    // Keep the alarm on for a short while so it is clearly visible
    delay(150);
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  // ---- Idle heartbeat line: prints only once per second while nothing happens,
  //      so you know the program is alive without flooding the monitor. ----
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();
    Serial.print("[");
    Serial.print(lastPrintTime);
    Serial.print(" ms] armed, events=");
    Serial.println(eventCount);
  }
}
