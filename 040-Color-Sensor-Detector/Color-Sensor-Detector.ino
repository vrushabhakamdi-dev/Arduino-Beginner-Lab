// 040 - Color Sensor Detector
// Reads red, green and blue reflection levels with a TCS3200 colour sensor
// and reports the dominant colour in the Serial Monitor.
// Board: Arduino Uno
// Wiring:
//   TCS3200 module ->  VCC = 5V, GND = GND
//                     S0 -> pin 4   (frequency scaling select)
//                     S1 -> pin 5
//                     S2 -> pin 6   (photodiode colour filter select)
//                     S3 -> pin 7
//                     OUT -> pin 8  (square wave output, frequency = light level)
//
// How the TCS3200 works:
//   The chip has an array of photodiodes, each covered by a red, green, blue or
//   clear filter. S2 and S3 choose which filter we listen to, and the OUT pin
//   then emits a square wave whose FREQUENCY is proportional to the amount of
//   that colour reaching the sensor. More light = higher frequency = shorter
//   pulse measured by pulseIn().
//
// *** WHITE-BALANCE CALIBRATION STEP ***
//   No two LEDs or daylight sources are equal, so the sensor sees "white" as
//   slightly tinted. At start-up this sketch asks you to hold a plain WHITE
//   card at the sensor, then records the red, green and blue frequencies it
//   sees. Every later reading is compared against those numbers, which is what
//   makes the colour names accurate. Re-run the sketch whenever you change the
//   lighting or the working distance.

#include <Arduino.h>   // Included so the sketch is explicit about its core helpers

// ---- Pin constants ----
const int S0_PIN = 4;   // Frequency scaling: 20% scaling when S0=HIGH, S1=LOW
const int S1_PIN = 5;
const int S2_PIN = 6;   // Filter selection with S3
const int S3_PIN = 7;
const int OUT_PIN = 8;  // Square wave output from the sensor

// ---- Timing constants ----
const unsigned long PULSE_TIMEOUT_US = 50000;  // Give up after 50 ms per pulse
const unsigned long SAMPLE_MS = 300;           // One full colour read per 300 ms
const int CAL_SAMPLES = 8;                     // Averages taken during calibration

// Calibration values (frequency in Hz) for a white reference card
unsigned long calRed = 0;
unsigned long calGreen = 0;
unsigned long calBlue = 0;

unsigned long lastSample = 0;

void setup() {
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  Serial.begin(9600);
  Serial.println("Color Sensor Detector starting...");

  // 20% output frequency scaling keeps pulse widths comfortably long for pulseIn()
  digitalWrite(S0_PIN, HIGH);
  digitalWrite(S1_PIN, LOW);

  calibrateWhite();
}

void loop() {
  if (millis() - lastSample < SAMPLE_MS) {
    return;
  }
  lastSample = millis();

  // ---- Measure one frequency for each colour filter ----
  unsigned long rawRed = readFrequency(false, false);   // S2=LOW,  S3=LOW  -> red
  unsigned long rawGreen = readFrequency(false, true);  // S2=LOW,  S3=HIGH -> green
  unsigned long rawBlue = readFrequency(true, true);    // S2=HIGH, S3=HIGH -> blue

  // ---- Convert to percentages of the white reference so colours are comparable
  float redPct = percentOf(rawRed, calRed);
  float greenPct = percentOf(rawGreen, calGreen);
  float bluePct = percentOf(rawBlue, calBlue);

  // ---- Decide which colour is dominant ----
  String colour = detectColour(redPct, greenPct, bluePct);

  // ---- Report everything so you can see both raw and scaled numbers ----
  Serial.print("R=");
  Serial.print(rawRed);
  Serial.print(" Hz  G=");
  Serial.print(rawGreen);
  Serial.print(" Hz  B=");
  Serial.print(rawBlue);
  Serial.print(" Hz   [R%");
  Serial.print(redPct, 0);
  Serial.print(" G%");
  Serial.print(greenPct, 0);
  Serial.print(" B%");
  Serial.print(bluePct, 0);
  Serial.print("]  -> ");
  Serial.println(colour);

  delay(50);   // Small settling gap between filters
}

// Selects a colour filter on S2/S3 and returns the OUT frequency in hertz.
unsigned long readFrequency(bool s2, bool s3) {
  digitalWrite(S2_PIN, s2);
  digitalWrite(S3_PIN, s3);
  delayMicroseconds(200);   // Let the new filter settle before measuring

  // pulseIn() waits for a full LOW pulse and returns its length in microseconds.
  // A longer pulse means a lower frequency, which means less light of that colour.
  unsigned long pulseWidth = pulseIn(OUT_PIN, LOW, PULSE_TIMEOUT_US);

  if (pulseWidth == 0) {
    return 0;   // Timed out: nothing reflected (or the sensor is not powered)
  }
  return 1000000UL / pulseWidth;   // microseconds -> hertz
}

// Averages several readings so a single noisy sample cannot ruin the number.
unsigned long averageFrequency(bool s2, bool s3) {
  unsigned long total = 0;
  int valid = 0;
  for (int i = 0; i < CAL_SAMPLES; i++) {
    unsigned long value = readFrequency(s2, s3);
    if (value > 0) {
      total += value;
      valid++;
    }
    delay(20);
  }
  if (valid == 0) return 0;
  return total / valid;
}

// Asks the user for a white card and records the reference frequencies.
void calibrateWhite() {
  Serial.println("CALIBRATION: hold a plain WHITE card about 2 cm above the sensor.");
  Serial.println("Calibrating in 5 seconds...");

  // Countdown gives you time to position the card
  for (int i = 5; i > 0; i--) {
    Serial.println(i);
    delay(1000);
  }

  calRed = averageFrequency(false, false);
  calGreen = averageFrequency(false, true);
  calBlue = averageFrequency(true, true);

  if (calRed == 0 || calGreen == 0 || calBlue == 0) {
    // Something is wrong, so fall back to fixed reference values.
    // The colours will be less accurate but the sketch still runs.
    Serial.println("WARNING: calibration failed, using default references.");
    calRed = 6000;
    calGreen = 6000;
    calBlue = 6000;
  } else {
    Serial.print("White reference captured: R=");
    Serial.print(calRed);
    Serial.print(" G=");
    Serial.print(calGreen);
    Serial.print(" B=");
    Serial.println(calBlue);
  }

  Serial.println("Ready. Show an object to the sensor.");
}

// Safe division: returns 0..100+ as a percentage of the white reference.
float percentOf(unsigned long value, unsigned long reference) {
  if (reference == 0) return 0;
  return (100.0 * (float)value) / (float)reference;
}

// Compares the three normalised channels and names the dominant colour.
String detectColour(float r, float g, float b) {
  // If almost nothing is reflected, we cannot trust the ratio at all
  if (r + g + b < 60.0) {
    return "UNKNOWN (too dark)";
  }

  float highest = r;
  String name = "RED";
  if (g > highest) { highest = g; name = "GREEN"; }
  if (b > highest) { highest = b; name = "BLUE"; }

  float lowest = r;
  if (g < lowest) lowest = g;
  if (b < lowest) lowest = b;

  // All three channels close together means no single colour dominates,
  // which is what a white (or grey) surface looks like.
  if (highest - lowest < 25.0) {
    return "WHITE";
  }
  return name;
}
