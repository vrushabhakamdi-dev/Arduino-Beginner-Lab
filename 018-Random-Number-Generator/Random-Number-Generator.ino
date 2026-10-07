// 018 - Random Number Generator
// Board: Arduino Uno
// Wiring:
//   Button -> one leg to pin 2, other leg to GND (internal pull-up)
//   LED    -> pin 13 through a 220 ohm resistor (built-in LED works too)
//   Analog pin A0 is left UNCONNECTED. The floating pin picks up electrical
//   noise, which is used to seed the random number generator in setup().
// Press the button to draw a new random number between 1 and 1000.

const int BUTTON_PIN = 2;        // Button that draws a new number
const int LED_PIN = 13;          // LED that flickers while "thinking"
const int SEED_PIN = A0;         // Floating analog pin used as the random seed
const int MIN_NUMBER = 1;        // Smallest number that can be drawn
const int MAX_NUMBER = 1000;     // Largest number that can be drawn
const unsigned long DEBOUNCE_MS = 30;   // Press must stay stable this long
const unsigned long FLICKER_MS = 25;    // Delay between LED flickers
const int FLICKER_COUNT = 12;           // Number of flickers per draw

// Debounce bookkeeping
int buttonRaw = HIGH, buttonStable = HIGH;
unsigned long buttonLastChange = 0;
int drawCount = 0;               // How many numbers have been drawn

// Returns true once each time the button is pressed
bool buttonPressed() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != buttonRaw) {    // Contact bounced, restart the stability timer
    buttonRaw = reading;
    buttonLastChange = millis();
  }
  if (buttonRaw != buttonStable && (millis() - buttonLastChange) >= DEBOUNCE_MS) {
    buttonStable = buttonRaw;
    if (buttonStable == LOW) return true;  // LOW means pressed (INPUT_PULLUP)
  }
  return false;
}

// Short "working" animation shown while the new number is being produced
void flickerLed() {
  for (int i = 0; i < FLICKER_COUNT; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(FLICKER_MS);
    digitalWrite(LED_PIN, LOW);
    delay(FLICKER_MS);
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);

  // Seed the generator once, using the noise on the floating pin A0.
  // Without randomSeed() the board would repeat exactly the same numbers
  // after every reset, because random() always starts from the same seed.
  randomSeed(analogRead(SEED_PIN));

  Serial.println("=== Random Number Generator ===");
  Serial.print("Press the button to generate a number from ");
  Serial.print(MIN_NUMBER);
  Serial.print(" to ");
  Serial.print(MAX_NUMBER);
  Serial.println(".");
}

void loop() {
  if (buttonPressed()) {
    drawCount++;
    flickerLed();

    // random(min, max) returns a value from min up to (but not including) max,
    // so MAX_NUMBER + 1 is used to include 1000 itself.
    int number = random(MIN_NUMBER, MAX_NUMBER + 1);

    Serial.print("Draw #");
    Serial.print(drawCount);
    Serial.print(" -> ");
    Serial.println(number);
  }
}
