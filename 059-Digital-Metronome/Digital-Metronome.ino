// 059 - Digital Metronome
// A steady beat-keeper for musicians. The tempo is set in BPM with two buttons,
// a third button starts or stops it, and every beat is announced by a tick on
// the buzzer plus a flash of the built-in LED. The fourth beat is accented with
// a higher pitch so you can count bars of four. Everything is timed with
// millis(), so nothing blocks and the buttons stay responsive.
// Board: Arduino Uno
//
// Wiring:
//   Tempo +5 BPM button -> pin 2   (shorts the pin to GND, pull-up enabled)
//   Tempo -5 BPM button -> pin 3
//   Start / stop button -> pin 4
//   Passive buzzer (+) -> pin 8, buzzer (-) -> GND
//   Built-in LED -> pin 13 (already on the board)

// ------------------------------ Pin constants ------------------------------
const byte BPM_UP_PIN = 2;    // button that raises the tempo
const byte BPM_DOWN_PIN = 3;  // button that lowers the tempo
const byte START_STOP_PIN = 4;// button that starts or stops the beat
const byte BUZZER_PIN = 8;    // passive buzzer plays the ticks
const byte LED_PIN = 13;      // built-in LED flashes on every beat

// ------------------------------ Tempo constants ----------------------------
const int MIN_BPM = 40;       // slowest allowed tempo
const int MAX_BPM = 220;      // fastest allowed tempo
const int BPM_STEP = 5;       // how much each press changes the tempo
const int DEFAULT_BPM = 100;  // starting tempo

// ------------------------------ Sound / flash constants --------------------
const int NORMAL_TICK_HZ = 1000;  // pitch of beats 1, 2 and 3
const int ACCENT_TICK_HZ = 1800;  // higher pitch on beat 4 (the accent)
const unsigned int TICK_MS = 30;  // length of each tick sound
const unsigned long LED_FLASH_MS = 50;   // how long the LED stays lit per beat
const unsigned long DEBOUNCE_MS = 30;

// ------------------------------ Runtime state ------------------------------
int bpm = DEFAULT_BPM;
bool running = false;              // true while the metronome is ticking
byte beatNumber = 0;               // counts 0,1,2,3 then wraps around
unsigned long beatIntervalMs = 0;  // milliseconds between beats (60000 / BPM)
unsigned long nextBeatAt = 0;      // millis() time of the next tick
unsigned long ledOffAt = 0;        // when to switch the flash LED back off

// Debounce state for the three buttons
const byte BUTTON_TOTAL = 3;
const byte BUTTON_PIN[BUTTON_TOTAL] = {BPM_UP_PIN, BPM_DOWN_PIN, START_STOP_PIN};
bool buttonRaw[BUTTON_TOTAL];
bool buttonStable[BUTTON_TOTAL];
unsigned long buttonLastChange[BUTTON_TOTAL];

// Returns the index (0-2) of a button that was just pressed, or -1
int readButtonPress(unsigned long now) {
  int found = -1;

  for (byte i = 0; i < BUTTON_TOTAL; i++) {
    bool reading = digitalRead(BUTTON_PIN[i]);  // LOW = pressed

    if (reading != buttonRaw[i]) {
      buttonRaw[i] = reading;
      buttonLastChange[i] = now;
    }

    if ((now - buttonLastChange[i]) >= DEBOUNCE_MS && reading != buttonStable[i]) {
      buttonStable[i] = reading;
      if (buttonStable[i] == LOW && found == -1) found = i;
    }
  }
  return found;
}

void updateInterval() {
  beatIntervalMs = 60000UL / (unsigned long)bpm;  // 60000 ms in a minute / BPM
}

void printTempo() {
  Serial.print("Tempo: ");
  Serial.print(bpm);
  Serial.print(" BPM   Beat interval: ");
  Serial.print(beatIntervalMs);
  Serial.println(" ms");
}

void startMetronome(unsigned long now) {
  running = true;
  beatNumber = 0;
  nextBeatAt = now;   // first tick happens immediately
  Serial.println("Metronome STARTED");
  printTempo();
}

void stopMetronome() {
  running = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, LOW);
  Serial.println("Metronome STOPPED");
}

// One beat: higher pitch every 4th beat, LED flash, then schedule the next beat
void playBeat(unsigned long now) {
  beatNumber++;
  if (beatNumber > 4) beatNumber = 1;   // count 1, 2, 3, 4, 1, 2, 3, 4...

  bool accent = (beatNumber == 4);
  tone(BUZZER_PIN, accent ? ACCENT_TICK_HZ : NORMAL_TICK_HZ, TICK_MS);

  digitalWrite(LED_PIN, HIGH);
  ledOffAt = now + LED_FLASH_MS;

  nextBeatAt = now + beatIntervalMs;  // non-blocking: no delay() anywhere here

  Serial.print("Beat ");
  Serial.print(beatNumber);
  Serial.print(accent ? " *ACCENT*  " : "  ");
  Serial.print(bpm);
  Serial.println(" BPM");
}

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < BUTTON_TOTAL; i++) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);  // buttons only need to touch GND
    buttonRaw[i] = digitalRead(BUTTON_PIN[i]);
    buttonStable[i] = buttonRaw[i];
    buttonLastChange[i] = millis();
  }

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  updateInterval();

  Serial.println("=== Digital Metronome ===");
  Serial.println("pin 2 = +5 BPM, pin 3 = -5 BPM, pin 4 = start/stop");
  Serial.println("Default tempo:");
  printTempo();
}

void loop() {
  unsigned long now = millis();

  // --- Button handling is checked on every pass of the loop ---
  int pressed = readButtonPress(now);

  if (pressed == 0) {                       // tempo up
    bpm += BPM_STEP;
    if (bpm > MAX_BPM) bpm = MAX_BPM;
    updateInterval();
    printTempo();
  } else if (pressed == 1) {                // tempo down
    bpm -= BPM_STEP;
    if (bpm < MIN_BPM) bpm = MIN_BPM;
    updateInterval();
    printTempo();
  } else if (pressed == 2) {                // start / stop
    if (running) stopMetronome();
    else startMetronome(now);
  }

  // --- Timing is done with millis(), so the loop is never blocked ---
  if (running && now >= nextBeatAt) {
    playBeat(now);
  }

  // Turn the flash LED off again once its time is up
  if (digitalRead(LED_PIN) == HIGH && now >= ledOffAt) {
    digitalWrite(LED_PIN, LOW);
  }
}
