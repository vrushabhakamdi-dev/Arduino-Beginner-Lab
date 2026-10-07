// 035 - Arduino Digital Compass
// Reads an HMC5883L magnetometer over I2C and prints a compass heading.
// Board: Arduino Uno
// Wiring (I2C bus):
//   HMC5883L VIN -> 5V (use 3.3V if your module has no on-board regulator)
//   HMC5883L GND -> GND
//   HMC5883L SDA -> pin A4  (Uno I2C data)
//   HMC5883L SCL -> pin A5  (Uno I2C clock)
//
// Libraries needed (install from the Arduino Library Manager):
//   "Adafruit HMC5883 Unified" by Adafruit
//   "Adafruit Unified Sensor"  by Adafruit   (dependency)
//   "Adafruit BusIO"           by Adafruit   (dependency)
//
// TIP: hold the board flat and away from motors, screws and magnets.
// Tilting the board strongly changes the reading, so keep it level.

#include <Wire.h>                    // Built-in library for I2C communication
#include <Adafruit_Sensor.h>         // Adafruit Unified Sensor abstraction
#include <Adafruit_HMC5883_U.h>      // Driver for the HMC5883L magnetometer

// Create the sensor object. The number 12345 is just a unique ID for this instance.
Adafruit_HMC5883_Unified compass = Adafruit_HMC5883_Unified(12345);

// Earth's magnetic field is roughly this strong in microtesla at the surface.
// It is only used to scale the printout, so a rough value is fine.
const float EARTH_FIELD_UT = 50.0;

// Store the last heading so we can show how much it moved
float lastHeading = 0.0;

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino Digital Compass starting...");

  // Try to talk to the sensor; stop with a clear message if it is not wired right
  if (!compass.begin()) {
    Serial.println("ERROR: HMC5883L not found. Check SDA=A4, SCL=A5 and power.");
    while (true) { delay(1000); }   // Halt forever - nothing else can run
  }

  // Ask the library for the sensor range and display it (handy for debugging)
  sensor_t sensor;
  compass.getSensor(&sensor);
  Serial.print("Sensor: ");
  Serial.println(sensor.name);
  Serial.println("Hold the board FLAT. Heading in degrees + direction:");
}

void loop() {
  sensors_event_t event;
  compass.getEvent(&event);   // Fill 'event' with magnetometer data in microtesla

  // ---- Compute the heading ----
  // atan2(y, x) gives the angle of the magnetic vector in the horizontal plane.
  // We subtract 90 degrees because the sensor's X axis points sideways relative
  // to the front of a typical breakout board. Adjust this offset for your mount.
  float heading = atan2(event.magnetic.y, event.magnetic.x) * 180.0 / PI - 90.0;

  // atan2() returns -180..+180, so shift it into the familiar 0..360 range
  if (heading < 0) {
    heading += 360.0;
  }
  if (heading >= 360.0) {
    heading -= 360.0;
  }

  // ---- Convert the angle into a 16-point direction name ----
  const char* direction = directionName(heading);

  // Show the raw magnetic strength too - near zero means a bad reading
  float magnitude = sqrt(event.magnetic.x * event.magnetic.x +
                         event.magnetic.y * event.magnetic.y +
                         event.magnetic.z * event.magnetic.z);

  // ---- Change since the last reading, wrapped into -180..+180 degrees ----
  // Without this, going from 359 deg to 1 deg would look like a 358 deg jump.
  float delta = heading - lastHeading;
  if (delta > 180.0)  delta -= 360.0;
  if (delta < -180.0) delta += 360.0;

  Serial.print("Heading: ");
  Serial.print(heading, 1);          // 1 decimal place
  Serial.print(" deg  ");
  Serial.print(direction);           // N, NE, E, SE, ...
  Serial.print("  field=");
  Serial.print(magnitude, 2);
  Serial.print(" uT  (Earth ref ");
  Serial.print(EARTH_FIELD_UT, 0);
  Serial.print(" uT)  change=");
  Serial.print(delta, 1);
  Serial.println(" deg");

  lastHeading = heading;
  delay(500);   // Two readings per second keeps the Serial Monitor readable
}

// Maps a heading in degrees to one of the 16 compass direction labels.
const char* directionName(float heading) {
  if (heading < 11.25 || heading >= 348.75) return "N";
  if (heading < 33.75)  return "NNE";
  if (heading < 56.25)  return "NE";
  if (heading < 78.75)  return "ENE";
  if (heading < 101.25) return "E";
  if (heading < 123.75) return "ESE";
  if (heading < 146.25) return "SE";
  if (heading < 168.75) return "SSE";
  if (heading < 191.25) return "S";
  if (heading < 213.75) return "SSW";
  if (heading < 236.25) return "SW";
  if (heading < 258.75) return "WSW";
  if (heading < 281.25) return "W";
  if (heading < 303.75) return "WNW";
  if (heading < 326.25) return "NW";
  return "NNW";
}
