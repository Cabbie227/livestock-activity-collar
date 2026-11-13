# Livestock Activity Collar

Arduino project for ELT.123 Electronic Components and Circuits.

A collar with an MPU6050 accelerometer that classifies an animal's activity as **resting**, **grazing** or **walking**, logs it over Serial, and raises an LED + buzzer alarm when the animal has been inactive for too long.

![Schematic](docs/schematic.png)

Simulated in Wokwi (red LED and buzzer = inactivity alarm):

![Wokwi simulation](docs/wokwi-circuit.png)

## Contents

| Path | Description |
|---|---|
| [`docs/REPORT.md`](docs/REPORT.md) | Project progress report (introduction, methods, results) |
| [`docs/REPORT.pdf`](docs/REPORT.pdf) | The same report as PDF |
| [`docs/presentation.pdf`](docs/presentation.pdf) | Slides for the 5-minute presentation |
| [`docs/PRESENTATION.md`](docs/PRESENTATION.md) | Speaking outline for the presentation |
| [`docs/schematic.png`](docs/schematic.png) | Circuit schematic |
| [`firmware/livestock_collar/`](firmware/livestock_collar) | Arduino sketch and classifier |
| [`simulation/diagram.json`](simulation/diagram.json) | Circuit for the Wokwi simulator |
| [`test/test_activity.cpp`](test/test_activity.cpp) | PC test of the classifier |

## Components

- Arduino Uno (or Nano)
- MPU6050 module (GY-521)
- Green LED, red LED, 2 × 220 Ω resistors
- Piezo buzzer
- 9 V battery

## Wiring

| Arduino | Component |
|---|---|
| 5V / GND | MPU6050 VCC / GND |
| A4 / A5 | MPU6050 SDA / SCL |
| D5 | 220 Ω → green LED → GND |
| D6 | 220 Ω → red LED → GND |
| D8 | Buzzer → GND |

## Running

**On hardware:** open `firmware/livestock_collar/livestock_collar.ino` in the Arduino IDE, select the board and port, upload, and open the Serial Monitor at 9600 baud. No extra libraries are needed.

**In Wokwi:** create a new Arduino Uno project at https://wokwi.com, paste the contents of `livestock_collar.ino` into `sketch.ino`, add `activity.h` as a new file, and replace `diagram.json` with `simulation/diagram.json`. Click the MPU6050 during simulation to change the acceleration.

**Classifier test on a PC:**

```bash
g++ -O2 -o test_activity test/test_activity.cpp && ./test_activity
```

## Output format

```
time_s,activity,intensity_g,head_angle_deg,temp_c,alert
2,RESTING,0.004,3.1,24.6,0
4,WALKING,0.231,6.4,24.6,0
```
