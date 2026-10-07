// 028 - Sound Level Meter
// Reads an analog sound sensor, maps the value to a 0-100 level with a
// peak-hold indicator, and lights an LED whenever the level goes above 70.
// Board: Arduino Uno
// Wiring:
//   Sound sensor module:
//     VCC -> 5V
//     GND -> GND
//     AO  (analog output) -> analog pin A0
//     Some boards also have a DO digital output and a small trimmer;
//     this project uses the analog output only, so DO can stay unconnected.
//     Turn the module's sensitivity trimmer to about mid position first.
//   LED -> digital pin 13 through a 220 ohm resistor (pin 13 is the
//     built-in LED on the Uno, so an external LED is optional).

// ---------------- Pin constants ----------------
const int SOUND_SENSOR_PIN = A0;  // Analog output of the sound sensor
const int LEVEL_LED_PIN = 13;     // Lights up when the level exceeds the limit

// ---------------- Level constants ----------------
const int ADC_MAX = 1023;            // Full-scale 10-bit ADC reading
const int PEAK_THRESHOLD = 70;       // Level above which the LED turns on
const unsigned long SAMPLE_MS = 50;  // One sample every 50 ms
const unsigned long PEAK_HOLD_MS = 1500;  // Peak stays visible for 1.5 seconds

int currentLevel = 0;      // Latest mapped level, 0 to 100
int peakLevel = 0;         // Highest level seen during the hold window
unsigned long peakTime = 0;  // When that peak was recorded
unsigned long lastSampleTime = 0;

void setup() {
  pinMode(LEVEL_LED_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Sound Level Meter starting...");
  Serial.println("Make a noise near the sensor and watch LEVEL and PEAK.");
}

// Decays the held peak a little whenever the hold time expires,
// so the display slowly falls back instead of staying stuck at the top
void updatePeak() {
  if (millis() - peakTime >= PEAK_HOLD_MS && peakLevel > 0) {
    peakLevel -= 1;      // Step the peak down one point
    peakTime = millis(); // Restart the decay step timer
  }
}

void loop() {
  // Take a sample on a fixed schedule
  if (millis() - lastSampleTime >= SAMPLE_MS) {
    lastSampleTime = millis();

    // Read the raw sound level from the sensor's analog output
    int rawValue = analogRead(SOUND_SENSOR_PIN);

    // Convert 0-1023 into a friendlier 0-100 scale
    currentLevel = map(rawValue, 0, ADC_MAX, 0, 100);

    // Keep the highest value reached inside the hold window
    if (currentLevel >= peakLevel) {
      peakLevel = currentLevel;
      peakTime = millis();
    }

    // Light the LED whenever the sound level passes the chosen limit
    if (currentLevel >= PEAK_THRESHOLD) {
      digitalWrite(LEVEL_LED_PIN, HIGH);
    } else {
      digitalWrite(LEVEL_LED_PIN, LOW);
    }

    // Print the current level and the held peak side by side
    Serial.print("LEVEL: ");
    Serial.print(currentLevel);
    Serial.print("  PEAK: ");
    Serial.print(peakLevel);

    // Show a matching bar so the level can be read at a glance
    Serial.print("  [");
    for (int i = 0; i < 50; i++) {
      if (i < (currentLevel / 2)) {
        Serial.print("#");
      } else {
        Serial.print("-");
      }
    }
    Serial.println("]");

    // Let the peak fall gradually after the sound stops
    updatePeak();
  }
}
