// 012 - Morse Code Flasher
// Board: Arduino Uno
// Wiring:
//   LED                -> pin 13 through a 220 ohm resistor (built-in LED works too)
//   Passive buzzer     -> (+) to pin 8, (-) to GND
// Open the Serial Monitor at 9600 baud, type a message (A-Z, 0-9, space) and
// press Send. The sketch flashes the message in Morse on the LED and plays the
// same dots and dashes as tones on the buzzer.

// ---------------- Pin and tone constants ----------------
const int LED_PIN = 13;
const int BUZZER_PIN = 8;
const unsigned int TONE_HZ = 700;     // Pitch played for every dot and dash

// ---------------- Morse timing (milliseconds) ----------------
// Standard Morse uses one unit for a dot, three units for a dash,
// one unit between elements, three units between letters and seven between words.
const unsigned int DOT_MS = 200;                // One "dot" = 1 unit
const unsigned int DASH_MS = DOT_MS * 3;        // One "dash" = 3 units
const unsigned int ELEMENT_GAP_MS = DOT_MS;     // Gap after each dot/dash = 1 unit
const unsigned int LETTER_GAP_MS = DOT_MS * 2;  // Extra gap after a letter (3 total)
const unsigned int WORD_GAP_MS = DOT_MS * 4;    // Extra gap after a space (7 total)

// Morse patterns for the letters A to Z
const char* LETTER_CODES[26] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--.."
};

// Morse patterns for the digits 0 to 9
const char* DIGIT_CODES[10] = {
  "-----", ".----", "..---", "...--", "....-",
  ".....", "-....", "--...", "---..", "----."
};

// Returns the Morse pattern for a character, or "" when it is not supported
String morseFor(char c) {
  if (c >= 'a' && c <= 'z') c = c - ('a' - 'A');  // change lower case to upper case
  if (c >= 'A' && c <= 'Z') return String(LETTER_CODES[c - 'A']);
  if (c >= '0' && c <= '9') return String(DIGIT_CODES[c - '0']);
  return "";
}

// Lights the LED and plays the tone for one dot or dash time
void sendUnit(unsigned int durationMs) {
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, TONE_HZ);
  delay(durationMs);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  delay(ELEMENT_GAP_MS);  // Required silent gap after every element
}

// Sends all dots and dashes of one character's pattern
void sendCode(const String &code) {
  for (unsigned int i = 0; i < code.length(); i++) {
    char symbol = code.charAt(i);
    if (symbol == '.') sendUnit(DOT_MS);
    else if (symbol == '-') sendUnit(DASH_MS);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("=== Morse Code Flasher ===");
  Serial.println("Type text (A-Z, 0-9, space) and press Send.");
  Serial.println("Each character is printed together with its Morse pattern.");
}

void loop() {
  if (Serial.available() > 0) {            // Is there a character waiting?
    char incoming = Serial.read();

    if (incoming == '\n' || incoming == '\r') return;  // Ignore line endings

    if (incoming == ' ') {                 // Word gap between two words
      Serial.println("  [word gap]");
      delay(WORD_GAP_MS);
      return;
    }

    String code = morseFor(incoming);
    if (code.length() == 0) {              // Character is not in our tables
      Serial.print("Unsupported character ignored: ");
      Serial.println(incoming);
      return;
    }

    // Show the mapping so the pattern can be learned while it flashes
    char shown = incoming;
    if (shown >= 'a' && shown <= 'z') shown = shown - ('a' - 'A');
    Serial.print(shown);
    Serial.print(" = ");
    Serial.println(code);

    sendCode(code);
    delay(LETTER_GAP_MS);  // Already had 1 unit after the last element, so 3 in total
  }
}
