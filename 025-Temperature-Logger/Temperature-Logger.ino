// 025 - Temperature Logger
// Reads a TMP36 analog temperature sensor and logs timestamped temperature
// readings as CSV lines in the Serial Monitor.
// Board: Arduino Uno
// Wiring:
//   TMP36 (flat face toward you, pins left to right): Vout, Vs, GND
//   TMP36 Vout -> analog pin A0
//   TMP36 Vs   -> 5V
//   TMP36 GND  -> GND
//
// IMPORTANT - sensor warm-up:
//   The TMP36 needs a short time after power-up before its readings settle.
//   Wait about 30 seconds after plugging in the board before trusting the
//   numbers, and avoid touching the sensor body with your fingers because
//   body heat will raise the reading by several degrees.

// Pin constant: the TMP36 analog output is on pin A0
const int TEMP_SENSOR_PIN = A0;

// Reference voltage of the Arduino Uno ADC and the TMP36's 0.5 V offset
const float VCC_VOLTS = 5.0;
const float ADC_RESOLUTION = 1023.0;   // 10-bit ADC: 0..1023 steps
const float TMP36_OFFSET_VOLTS = 0.5;  // Sensor outputs 0.5 V at 0 degrees C
const float MV_PER_DEGREE = 10.0;      // Sensor changes by 10 mV per degree C

// Logging interval: one CSV row every 2 seconds
const unsigned long LOG_INTERVAL_MS = 2000;

unsigned long lastLogTime = 0;

void setup() {
  // The TMP36 is an analog device, so A0 is used as an input by default
  pinMode(TEMP_SENSOR_PIN, INPUT);

  Serial.begin(9600);
  Serial.println("Temperature Logger starting...");
  Serial.println("Let the sensor warm up for about 30 seconds before logging.");
  // Print a header row so the data can be pasted straight into a spreadsheet
  Serial.println("millis,degrees_c,degrees_f");
}

// Converts one ADC sample into degrees Celsius using the TMP36 formula
float readTemperatureC() {
  int adcValue = analogRead(TEMP_SENSOR_PIN);

  // Step 1: turn the 0-1023 reading into a voltage between 0 and 5 V
  float volts = adcValue * (VCC_VOLTS / ADC_RESOLUTION);

  // Step 2: remove the sensor's 0.5 V offset at 0 degrees C
  float voltsAboveZero = volts - TMP36_OFFSET_VOLTS;

  // Step 3: 10 mV equals 1 degree, so divide by 0.01 to get degrees Celsius
  return voltsAboveZero / (MV_PER_DEGREE / 100.0);
}

void loop() {
  // Log on a fixed schedule instead of blocking with delay()
  if (millis() - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = millis();

    float tempC = readTemperatureC();

    // Fahrenheit conversion: F = C * 9/5 + 32
    float tempF = (tempC * 9.0 / 5.0) + 32.0;

    // CSV line: elapsed milliseconds, Celsius, Fahrenheit
    Serial.print(millis());
    Serial.print(",");
    Serial.print(tempC, 2);
    Serial.print(",");
    Serial.println(tempF, 2);
  }
}
