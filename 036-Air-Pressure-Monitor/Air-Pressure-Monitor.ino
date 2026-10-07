// 036 - Air Pressure Monitor
// Reads air pressure, temperature and estimated altitude from a BMP280 sensor.
// Board: Arduino Uno
// Wiring (I2C bus):
//   BMP280 VIN -> 5V (use 3.3V if your module has no on-board regulator)
//   BMP280 GND -> GND
//   BMP280 SDA -> pin A4  (Uno I2C data)
//   BMP280 SCL -> pin A5  (Uno I2C clock)
//   (some modules label the pins SDI/SCK instead of SDA/SCL)
//
// Libraries needed (install from the Arduino Library Manager):
//   "Adafruit BMP280" by Adafruit
//   "Adafruit BusIO"  by Adafruit   (dependency)

#include <Wire.h>              // Built-in I2C library
#include <Adafruit_BMP280.h>   // Driver for the BMP280 pressure/temperature sensor

Adafruit_BMP280 bmp;           // Sensor object using the default I2C address 0x76

// ---- Sea-level pressure constant (hPa) ----
// The altitude formula needs the pressure at sea level as a reference.
// 1013.25 hPa is the international standard. Change it to your local value
// (from a weather app or airport METAR) to get a more accurate altitude.
const float SEA_LEVEL_HPA = 1013.25;

// How often we print a new reading (milliseconds)
const unsigned long SAMPLE_MS = 1000;

unsigned long lastSample = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Air Pressure Monitor starting...");

  // Try to start the sensor on address 0x76, then fall back to 0x77
  if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
    Serial.println("ERROR: BMP280 not found. Check SDA=A4, SCL=A5 and power.");
    while (true) { delay(1000); }   // Halt forever - there is nothing to read
  }

  // ---- Configure how the sensor trades speed against noise ----
  // TEMPERATURE_OVERSAMPLING x2, PRESSURE_OVERSAMPLING x16, NORMAL_MODE keeps
  // measuring continuously so a fresh value is always ready.
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,    // temperature oversampling
                  Adafruit_BMP280::SAMPLING_X16,   // pressure oversampling
                  Adafruit_BMP280::FILTER_X16,      // noise filter
                  Adafruit_BMP280::STANDBY_MS_500); // time between measurements

  Serial.println("time_ms | pressure(hPa) | temp(degC) | altitude(m)");
}

void loop() {
  // Only print once per second so the Serial Monitor is easy to follow
  if (millis() - lastSample < SAMPLE_MS) {
    return;
  }
  lastSample = millis();

  // ---- Read the three values the BMP280 provides ----
  float pressureHPa = bmp.readPressure() / 100.0;  // Pascals -> hectopascals
  float temperatureC = bmp.readTemperature();      // Degrees Celsius
  float altitudeM = bmp.readAltitude(SEA_LEVEL_HPA); // Estimated height in metres

  // If the sensor is missing or mis-wired, readPressure() returns NaN (not a number)
  if (isnan(pressureHPa) || isnan(temperatureC)) {
    Serial.println("ERROR: bad reading from BMP280.");
    return;
  }

  // ---- Show the results ----
  Serial.print(millis());
  Serial.print(" | ");
  Serial.print(pressureHPa, 2);   // 2 decimal places
  Serial.print(" | ");
  Serial.print(temperatureC, 1);  // 1 decimal place
  Serial.print(" | ");
  Serial.println(altitudeM, 1);

  // A quick hint about weather trends: pressure rising usually means clearer
  // weather is on the way, while falling pressure often brings clouds and rain.
  static float previousHPa = 0;
  if (previousHPa != 0) {
    float drift = pressureHPa - previousHPa;
    if (drift > 0.15) {
      Serial.println("  -> pressure rising (often clearer weather)");
    } else if (drift < -0.15) {
      Serial.println("  -> pressure falling (often worsening weather)");
    }
  }
  previousHPa = pressureHPa;
}
