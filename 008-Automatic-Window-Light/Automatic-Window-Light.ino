// 008 - Automatic Window Light
// A light-dependent resistor (LDR) measures the room brightness and an
// LED switches on automatically when it gets dark, then switches off
// again when daylight returns. Hysteresis stops the LED flickering.
// Board: Arduino Uno
// Wiring: 5V -> LDR -> A0 -> 10k ohm resistor -> GND  (voltage divider)
//         LED anode -> pin 9 through a 220 ohm resistor, cathode -> GND

// Pin constants
const int LDR_PIN = A0;      // Middle point of the LDR / 10k divider
const int LED_PIN = 9;       // Output LED (PWM capable, so it also dims smoothly)

// Threshold constants (0-1023 ADC counts)
const int DARK_THRESHOLD = 400;   // Below this it is dark  -> LED turns ON
const int BRIGHT_THRESHOLD = 600; // Above this it is bright -> LED turns OFF
// The gap between 400 and 600 is the hysteresis band that prevents flicker.

bool lightOn = false;        // Current state of the automatic light

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Automatic Window Light starting...");
  Serial.println("Turn the room dark to switch the LED on.");
}

void loop() {
  // Take a few readings and average them to smooth out sensor noise
  long total = 0;
  for (int i = 0; i < 5; i++) {
    total += analogRead(LDR_PIN);
    delay(10);
  }
  int lightLevel = total / 5;   // Average of 5 samples, 0 (dark) to 1023 (bright)

  // Switch logic with hysteresis:
  // - if it is currently OFF, only turn ON when clearly dark (below 400)
  // - if it is currently ON, only turn OFF when clearly bright (above 600)
  if (!lightOn && lightLevel < DARK_THRESHOLD) {
    lightOn = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.print("It is dark (level ");
    Serial.print(lightLevel);
    Serial.println(") -> LED ON");
  } else if (lightOn && lightLevel > BRIGHT_THRESHOLD) {
    lightOn = false;
    digitalWrite(LED_PIN, LOW);
    Serial.print("It is bright (level ");
    Serial.print(lightLevel);
    Serial.println(") -> LED OFF");
  }

  delay(200);  // Poll 5 times a second, plenty fast for a room light
}
