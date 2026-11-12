# 5-minute presentation outline

**1. Title (20 s)**
Livestock Activity Collar — name, supervisor.

**2. Problem and application (50 s)**
- Sick animals move and eat less, but a farmer cannot watch a whole herd.
- A collar that measures activity gives an early warning.
- Uses: health monitoring, heat detection, calving alert, pasture management.

**3. Components and schematic (60 s)**
- Arduino Uno, MPU6050 accelerometer (I2C on A4/A5), two LEDs with 220 Ω resistors, buzzer, 9 V battery.
- Show `schematic.png`.

**4. How it works (90 s)**
- 20 Hz sampling, 2-second windows.
- Feature 1: movement intensity = standard deviation of acceleration magnitude.
- Feature 2: head-down angle from the direction of gravity.
- Rules: low intensity → resting; moving + head down → grazing; moving + head level → walking.
- No movement for too long → red LED and buzzer.

**5. Results so far (50 s)**
- Schematic and firmware finished.
- Classifier tested on a PC with synthetic signals: 600/600 windows correct.
- Not yet tested on real hardware or on an animal.

**6. Next steps (30 s)**
- Hardware build, real data and threshold tuning, wireless link, SD logging, smaller board and case.
