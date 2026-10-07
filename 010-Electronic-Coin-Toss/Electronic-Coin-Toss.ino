// 010 - Electronic Coin Toss
// Press the button and the Arduino flips a virtual coin: the green LED
// lights for HEADS, the red LED lights for TAILS, and the winner is
// printed to the Serial Monitor.
// Board: Arduino Uno
// Wiring: Push button between pin 2 and GND (internal pull-up used).
//         Green LED -> pin 5 through a 220 ohm resistor -> GND  (HEADS)
//         Red LED   -> pin 6 through a 220 ohm resistor -> GND  (TAILS)
//         A0 left unconnected: the floating pin picks up electrical noise
//         which is used to seed the random number generator.

// Pin constants
const int BUTTON_PIN = 2;   // Trigger input
const int GREEN_LED_PIN = 5; // Heads indicator
const int RED_LED_PIN = 6;   // Tails indicator

// Result constants
const int HEADS = 0;
const int TAILS = 1;

const unsigned long DEBOUNCE_MS = 50;  // Ignore switch bounce for 50 ms
const int SHOW_MS = 2500;              // How long the result LED stays lit

bool tossDone = false;               // Repeat guard: one toss per press
unsigned long lastChangeTime = 0;
int lastReading = HIGH;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Clear both indicators at start-up
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.begin(9600);

  // Seed the random generator from noise on the unconnected pin A0.
  // Without this the board would produce the same sequence every boot.
  randomSeed(analogRead(A0));

  Serial.println("Electronic Coin Toss ready. Press the button to flip!");
}

void loop() {
  // Debounced button read (INPUT_PULLUP means pressed == LOW)
  int reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (reading != lastReading) {
    lastChangeTime = now;
    lastReading = reading;
  }

  bool pressed = (reading == LOW) && ((now - lastChangeTime) >= DEBOUNCE_MS);

  if (pressed && !tossDone) {
    tossDone = true;

    // random(2) returns either 0 (HEADS) or 1 (TAILS)
    int coin = random(2);

    if (coin == HEADS) {
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);
      Serial.println("Coin flip: HEADS (green LED)");
    } else {
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      Serial.println("Coin flip: TAILS (red LED)");
    }

    // Hold the result, then switch both LEDs off again
    delay(SHOW_MS);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
  }

  // Clear the guard when the button is released so the next press works
  if (reading == HIGH) {
    tossDone = false;
  }
}
