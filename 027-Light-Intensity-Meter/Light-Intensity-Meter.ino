// 027 - Light Intensity Meter
// Measures ambient light with an LDR (light dependent resistor) and shows the
// result both as a raw value plus percentage and as a five-LED bar graph.
// Board: Arduino Uno
// Wiring:
//   Voltage divider on analog pin A0:
//     5V --- LDR --- A0 --- 10 kohm resistor --- GND
//   (More light = lower LDR resistance = higher voltage at A0 = higher reading)
//   LED bar, each with a 220 ohm resistor to GND:
//     LED 1 -> pin 9, LED 2 -> pin 10, LED 3 -> pin 11,
//     LED 4 -> pin 12, LED 5 -> pin 13
//
// No library is needed: an LDR is a plain analogue component.

// ---------------- Analog input ----------------
const int LDR_PIN = A0;   // Middle point of the LDR / 10 kohm divider

// ---------------- LED bar pins (5 LEDs from pin 9 to pin 13) ----------------
const int LED_BAR_PIN_1 = 9;   // Lowest light level
const int LED_BAR_PIN_2 = 10;
const int LED_BAR_PIN_3 = 11;
const int LED_BAR_PIN_4 = 12;
const int LED_BAR_PIN_5 = 13;  // Highest light level (pin 13 = built-in LED)

const int LED_BAR_PINS[5] = {LED_BAR_PIN_1, LED_BAR_PIN_2, LED_BAR_PIN_3,
                             LED_BAR_PIN_4, LED_BAR_PIN_5};

// ---------------- Measurement constants ----------------
const int ADC_MAX = 1023;              // Full-scale value of the 10-bit ADC
const unsigned long UPDATE_INTERVAL_MS = 200;  // Refresh every 200 ms

unsigned long lastUpdateTime = 0;

void setup() {
  // Configure all five bar LEDs as outputs using a loop
  for (int i = 0; i < 5; i++) {
    pinMode(LED_BAR_PINS[i], OUTPUT);
  }

  Serial.begin(9600);
  Serial.println("Light Intensity Meter starting...");
  Serial.println("raw\tpct\tbar");
}

// Lights the first 'level' LEDs (0 to 5) and switches the rest off
void showBarLevel(int level) {
  for (int i = 0; i < 5; i++) {
    if (i < level) {
      digitalWrite(LED_BAR_PINS[i], HIGH);
    } else {
      digitalWrite(LED_BAR_PINS[i], LOW);
    }
  }
}

void loop() {
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    lastUpdateTime = millis();

    // Raw 10-bit reading: 0 = total darkness, 1023 = brightest light
    int rawValue = analogRead(LDR_PIN);

    // Convert the raw count into a 0-100 percent value
    // map() is an Arduino built-in: map(value, fromLow, fromHigh, toLow, toHigh)
    int percent = map(rawValue, 0, ADC_MAX, 0, 100);

    // Turn the percentage into 0..5 lit LEDs for the bar graph
    int barLevel = map(rawValue, 0, ADC_MAX, 0, 5);
    showBarLevel(barLevel);

    // Print the raw value and the percentage, separated by tabs
    Serial.print(rawValue);
    Serial.print("\t");
    Serial.print(percent);
    Serial.print("%\t");

    // Draw a simple ASCII bar so the trend is obvious in the Serial Monitor
    for (int i = 0; i < barLevel; i++) {
      Serial.print("|");
    }
    Serial.println();
  }
}
