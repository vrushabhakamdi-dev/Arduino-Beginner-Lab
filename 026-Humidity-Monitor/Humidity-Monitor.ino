// 026 - Humidity Monitor
// Reads humidity (and temperature) from a DHT11 sensor and prints a simple
// comfort message based on the humidity level.
// Board: Arduino Uno
// Wiring:
//   DHT11 (front face with the grille, pins left to right): VCC, DATA, NC, GND
//   DHT11 VCC  -> 5V
//   DHT11 DATA -> digital pin 2  (with a 10 kohm pull-up resistor to 5V)
//   DHT11 NC   -> not connected
//   DHT11 GND  -> GND
//
// The 10 kohm pull-up resistor between DATA and 5V is required: it holds the
// data line high between the sensor's replies. Some DHT11 breakout boards have
// this resistor already fitted, in which case you can omit it.
//
// Libraries required (install both from the Library Manager):
//   "DHT sensor library" by Adafruit
//   "Adafruit Unified Sensor" (a dependency of the DHT library)

#include <DHT.h>      // Adafruit DHT11/DHT22 temperature-humidity library
#include <Adafruit_Sensor.h>  // Adafruit unified sensor abstraction layer

// ---------------- Pin and sensor constants ----------------
const int DHT_PIN = 2;            // Data pin of the DHT11
const int DHT_TYPE = DHT11;       // Change to DHT22 if you use that sensor

// ---------------- Timing constants ----------------
const unsigned long READ_INTERVAL_MS = 2000;  // Read every 2 seconds
// The DHT11 is slow: readings faster than about 1 second are ignored or
// return the previous value, so 2 seconds is a safe interval.

unsigned long lastReadTime = 0;

// Create the sensor object once; it handles all the signalling for us
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(9600);
  Serial.println("Humidity Monitor starting...");
  Serial.println("Give the sensor a few seconds to wake up before judging values.");

  // Start the DHT11 communication; this also configures the data pin
  dht.begin();
}

// Returns a short English description of the current humidity level
String comfortMessage(float humidity) {
  if (humidity < 30.0) {
    return "DRY - air may irritate skin and airways";
  } else if (humidity <= 60.0) {
    return "COMFORTABLE - good for people and electronics";
  } else {
    return "HUMID - risk of condensation and mould";
  }
}

void loop() {
  // Schedule the read with millis() so nothing blocks between readings
  if (millis() - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = millis();

    // readHumidity() returns NaN (Not a Number) if the sensor did not reply,
    // which usually means a loose wire or a missing pull-up resistor
    float humidity = dht.readHumidity();
    float temperatureC = dht.readTemperature();

    if (isnan(humidity) || isnan(temperatureC)) {
      Serial.println("ERROR: could not read from DHT11 - check wiring");
      return;
    }

    // Humidity percentage with no decimals (the DHT11 only resolves 1 % anyway)
    Serial.print("HUMIDITY: ");
    Serial.print(humidity, 1);
    Serial.print(" %  |  TEMP: ");
    Serial.print(temperatureC, 1);
    Serial.print(" C  |  ");
    Serial.println(comfortMessage(humidity));
  }
}
