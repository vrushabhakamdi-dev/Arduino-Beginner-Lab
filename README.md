# 100 Arduino Projects for Beginners 🚀

A complete collection of **100 unique, beginner-friendly Arduino projects** — each with working source code (`*.ino`) and clear documentation (`project_info.txt`). Build them in order and go from your very first LED blink to autonomous robots.

## 🎯 Introduction

This repository is designed to help beginners learn, hands-on:

- **Arduino programming** — C/C++ syntax, functions, loops, conditions
- **Electronics** — circuits, wiring, resistors, voltage dividers
- **Sensors** — ultrasonic, IR, temperature, humidity, gas, flame, motion, and more
- **Displays** — LCD, OLED, seven-segment, LED matrix
- **Motors** — servo, DC, stepper, relays
- **Automation** — timers, locks, alarms, dispensers, controllers
- **Robotics** — line followers, obstacle avoiders, sumo bots, robot arms
- **Embedded systems** — timing, state machines, interrupts, communication
- **Problem solving** — planning, debugging, and iterating on real builds

## 📁 Repository Structure

```text
100-Arduino-Projects/
│
├── README.md
│
├── 001-LED-Blink/
│   ├── LED-Blink.ino
│   └── project_info.txt
│
├── 002-Multiple-LED-Sequence/
│   ├── Multiple-LED-Sequence.ino
│   └── project_info.txt
│
...
│
└── 100-Arduino-Floor-Cleaner-Robot/
    ├── Floor-Cleaner-Robot.ino
    └── project_info.txt
```

Every project folder contains exactly two files:

1. **`*.ino`** — complete, commented, upload-ready Arduino sketch
2. **`project_info.txt`** — project idea, how it works, components, libraries, learning outcomes, upgrades

### ▶️ How to Open a Project

The `.ino` file name does not include the number prefix (per this repository's structure), so when Arduino IDE asks *"The file needs to be inside a sketch folder named X"* — choose to keep your files in place and simply **rename the `.ino` to match its folder name** (e.g. `001-LED-Blink.ino`) before uploading. All other Arduino IDE steps are standard: select **Tools → Board → Arduino Uno** and **Tools → Port**, then **Verify** and **Upload**.

---

## 📂 Project Categories

### 🔵 Arduino Fundamentals (001–020)
LEDs, buttons, buzzers, potentiometers, timing and binary — the building blocks of every project that follows.

### 🟢 Sensors & Measurements (021–040)
Distance, light, temperature, humidity, sound, flame, gas, rain, motion, magnetism, pressure, color — reading the physical world.

### 🟡 Displays, Controls & Games (041–060)
LCD/OLED screens, seven-segment displays, keypads, joysticks, menus, calculators, quizzes and electronic games.

### 🟠 Motors & Automation (061–080)
Servos, DC motors, steppers, relays, locks, timers, reminders and sensor-driven automatic systems.

### 🔴 Robotics & Advanced Beginner Projects (081–100)
Self-driving robots, robot arms, sorting machines, security systems and a complete floor-cleaner robot.

---

## 📊 Project Table

| # | Project | Difficulty | Main Concept |
| - | ------- | ---------- | ------------ |
| 001 | LED Blink | Beginner | Digital output & delay |
| 002 | Multiple LED Sequence | Beginner | Loops & LED sequencing |
| 003 | Traffic Light | Beginner | Timed state sequence |
| 004 | Push Button Counter | Beginner | Digital input & debouncing |
| 005 | Buzzer Doorbell | Beginner | Melodies with tone() |
| 006 | Potentiometer LED Dimmer | Beginner | Analog input & PWM |
| 007 | RGB LED Color Mixer | Beginner | PWM color mixing |
| 008 | Automatic Window Light | Beginner | LDR threshold switching |
| 009 | LED Dice | Beginner | Random numbers & patterns |
| 010 | Electronic Coin Toss | Beginner | Random two-outcome logic |
| 011 | Reaction Time Tester | Intermediate | millis() timing measurement |
| 012 | Morse Code Flasher | Intermediate | Serial input & text encoding |
| 013 | LED Binary Clock | Intermediate | Binary display & time logic |
| 014 | LED Bar Graph | Beginner | Analog-to-level mapping |
| 015 | Analog Voltage Indicator | Beginner | Voltage measurement |
| 016 | Digital Stopwatch | Intermediate | Elapsed time with millis() |
| 017 | Countdown Timer | Intermediate | Timer state machine |
| 018 | Random Number Generator | Beginner | randomSeed & Serial output |
| 019 | LED Heart Animation | Beginner | Animation frame arrays |
| 020 | Binary Counter with LEDs | Beginner | Counting in binary |
| 021 | Ultrasonic Distance Meter | Beginner | Ultrasonic pulse timing |
| 022 | Parking Distance Assistant | Intermediate | Proximity audio feedback |
| 023 | IR Object Counter | Beginner | Edge detection & counting |
| 024 | IR Remote LED Controller | Intermediate | IRremote decoding |
| 025 | Temperature Logger | Beginner | Analog temperature sensing |
| 026 | Humidity Monitor | Beginner | DHT11 sensor reading |
| 027 | Light Intensity Meter | Beginner | LDR measurement |
| 028 | Sound Level Meter | Beginner | Analog sound levels |
| 029 | Flame Detection Alarm | Beginner | Flame sensor alarm |
| 030 | Gas Leak Alarm | Intermediate | MQ-2 gas threshold alarm |
| 031 | Rain Detection Alarm | Beginner | Rain sensor triggering |
| 032 | Vibration Detection Alarm | Beginner | Vibration sensing |
| 033 | Magnetic Door Detector | Beginner | Reed switch monitoring |
| 034 | PIR Motion Detector | Beginner | PIR motion events |
| 035 | Arduino Digital Compass | Intermediate | Magnetometer heading |
| 036 | Air Pressure Monitor | Intermediate | Barometric pressure sensing |
| 037 | Digital Tilt Detector | Beginner | Tilt switch sensing |
| 038 | Rotary Encoder Position Meter | Intermediate | Encoder counting |
| 039 | Infrared Line Sensor Tester | Beginner | Reflectance sensing |
| 040 | Color Sensor Detector | Intermediate | Color recognition |
| 041 | LCD Digital Clock | Intermediate | RTC timekeeping |
| 042 | OLED Countdown Display | Intermediate | OLED graphics rendering |
| 043 | Seven Segment Counter | Intermediate | 74HC595 shift register |
| 044 | LED Matrix Scrolling Message | Intermediate | MAX7219 scrolling text |
| 045 | LCD Calculator | Intermediate | LCD menu & arithmetic |
| 046 | Keypad Calculator | Intermediate | Keypad input parsing |
| 047 | Keypad Digital Lock | Intermediate | PIN entry system |
| 048 | Joystick LED Controller | Beginner | Joystick analog control |
| 049 | Joystick Servo Controller | Beginner | Analog-to-angle mapping |
| 050 | Rotary Encoder Menu System | Intermediate | Menu navigation UI |
| 051 | Push Button Quiz Buzzer | Beginner | First-input priority logic |
| 052 | Electronic Number Guessing Game | Beginner | Serial game logic |
| 053 | Digital Roulette | Beginner | Sequential random stopping |
| 054 | Rock Paper Scissors Game | Beginner | Random game logic |
| 055 | LED Memory Pattern Game | Beginner | Simon-style memory play |
| 056 | Whack-a-Button Game | Beginner | Reaction timing game |
| 057 | Electronic Theremin | Intermediate | Light-controlled pitch |
| 058 | Mini Electronic Piano | Beginner | Multi-button tones |
| 059 | Digital Metronome | Intermediate | BPM timing with millis() |
| 060 | Adjustable Buzzer Tone Generator | Beginner | Variable frequency tones |
| 061 | Servo Sweep Tester | Beginner | Servo motor basics |
| 062 | Servo Controlled Gauge | Beginner | Analog-to-angle mapping |
| 063 | Servo Parking Barrier | Intermediate | Sensor-triggered servo |
| 064 | Servo Automatic Door | Intermediate | PIR-triggered actuator |
| 065 | DC Motor Speed Controller | Beginner | PWM motor speed |
| 066 | DC Motor Direction Controller | Beginner | H-bridge direction control |
| 067 | Stepper Motor Positioner | Intermediate | Stepper positioning |
| 068 | Stepper Motor Clock Hand | Intermediate | Timed precise stepping |
| 069 | Manual Fan Speed Controller | Beginner | Potentiometer fan control |
| 070 | Temperature-Controlled Fan | Intermediate | Temperature-based speed |
| 071 | Relay Controlled Lamp | Beginner | Relay switching |
| 072 | Automatic Curtain Prototype | Intermediate | Light-driven actuator |
| 073 | Programmable Appliance Timer | Intermediate | Countdown + relay control |
| 074 | Electronic Combination Lock | Advanced Beginner | Keypad lock + servo + EEPROM |
| 075 | Ultrasonic Garage Door | Intermediate | Distance-triggered door |
| 076 | Automatic Trash Can | Intermediate | IR-triggered servo lid |
| 077 | Electronic Pill Reminder | Intermediate | Scheduled reminders |
| 078 | Water Level Alarm | Beginner | Liquid level sensing |
| 079 | Water Tank Level Controller | Advanced Beginner | Level-based pump control |
| 080 | Automatic Hand Sanitizer Dispenser | Intermediate | Motion-triggered pump |
| 081 | Line Following Robot | Intermediate | Closed-loop line tracking |
| 082 | Obstacle Avoiding Robot | Intermediate | Autonomous navigation |
| 083 | Light Following Robot | Intermediate | Differential light steering |
| 084 | Wired Controlled Robot Car | Beginner | Tethered remote control |
| 085 | Sumo Robot | Advanced Beginner | Opponent & edge detection |
| 086 | Robot Arm Servo Controller | Intermediate | Multi-axis servo control |
| 087 | Automatic Object Sorting Machine | Advanced Beginner | Sensor-based sorting |
| 088 | Electronic Safe | Advanced Beginner | PIN + EEPROM + actuator |
| 089 | Motion Security Alarm | Intermediate | Armed PIR alarm system |
| 090 | Laser Tripwire Alarm | Intermediate | Beam-break detection |
| 091 | Fingerprint Access System | Advanced Beginner | Biometric authentication |
| 092 | Emergency Siren System | Beginner | Multi-tone siren patterns |
| 093 | Smoke Detection Alarm | Intermediate | Latching smoke alarm |
| 094 | Accident Impact Detector | Advanced Beginner | Accelerometer impact detection |
| 095 | Driver Alertness Alarm | Intermediate | Inattention timing alarm |
| 096 | Automatic Railway Crossing | Intermediate | Sensor-gated crossing |
| 097 | Electronic Queue Token System | Intermediate | Token & call workflow |
| 098 | Digital Weight Scale | Advanced Beginner | Load cell & HX711 |
| 099 | Frequency Meter | Advanced Beginner | Interrupt pulse counting |
| 100 | Arduino Floor Cleaner Robot | Advanced Beginner | Random-bounce cleaning robot |

---

## 🗺️ Learning Path

```text
Beginner
↓
LEDs & Buttons
↓
Sensors
↓
Displays
↓
Games
↓
Motors
↓
Automation
↓
Robotics
↓
Advanced Beginner
```

**Recommended progression:** finish each category before moving to the next. Every `project_info.txt` lists the difficulty and what you will learn, so you always know what comes next.

---

## 📜 Repository Rules

- ✅ **Arduino only** — Uno, Nano, Mega and Arduino-compatible modules
- ❌ **No IoT** — no ESP8266, no ESP32
- ❌ **No Wi-Fi, no Bluetooth**
- ❌ **No cloud platforms** — no Firebase, MQTT, Blynk, web dashboards, internet APIs
- ❌ **No smartphone-controlled systems**
- ❌ **Smart Plant Care System excluded** — no plant watering / irrigation projects of any kind (soil-moisture sensors appear only for measurement education)
- ✅ All projects run **locally on Arduino hardware**
- ⚠️ Alarm/security/safety projects are **educational prototypes** — never certified safety-critical systems

---

## 🛠️ Getting Started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Open any project's `.ino` file (see *How to Open a Project* above).
3. Check `project_info.txt` for required components and libraries.
4. Install libraries via **Sketch → Include Library → Manage Libraries…** (names are listed in `project_info.txt`).
5. Wire the circuit using the components list.
6. Select **Tools → Board** and **Tools → Port**, then **Verify** and **Upload**.
7. Open the **Serial Monitor** (9600 baud unless stated otherwise) where applicable.

Happy building! 🛠️
