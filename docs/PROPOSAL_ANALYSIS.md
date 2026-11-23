---
title: "Livestock Activity Collar — Original Proposal and Analysis of Outcomes"
---

**Course:** ELT.123 Electronic Components and Circuits  
**Student:** Nazar Smatov  **Supervisor:** Tauheed Khan  
**Repository:** <https://github.com/Cabbie227/livestock-activity-collar>  
**Date:** November 2026

# Part 1. Original proposal

The project was chosen from the course Project Bank (entry #25). The proposal as submitted:

| Field | Proposal |
|---|---|
| Sub-domain | Smart Agri |
| Title & objective | **Livestock Activity Collar:** classify grazing, rumination and resting behaviours |
| Research question | Can a 3-axis accelerometer classify cattle behaviours using low-power decision trees? |
| Methodology & tools | Neck-mounted MPU6050, Edge Impulse feature extraction, local classification and LoRa logging |
| Hardware availability | Yes |
| Presentation format | Poster |

**Goals derived from the proposal**

| # | Goal |
|---|---|
| G1 | Read 3-axis acceleration from a neck-mounted MPU6050 |
| G2 | Extract features from the acceleration signal |
| G3 | Classify **grazing, rumination and resting** |
| G4 | Run the classifier **locally** on the microcontroller as a **low-power decision tree** |
| G5 | Use **Edge Impulse** for feature extraction / model training |
| G6 | Send the results over **LoRa** |
| G7 | Build a working prototype that can be demonstrated |

# Part 2. What was built

An Arduino Uno reads an MPU6050 over I2C at 20 Hz. Every 4 seconds (80 samples) it computes three features — **movement intensity** (standard deviation of the acceleration magnitude), **head-down angle** (from the direction of gravity) and **rhythm** (autocorrelation of the magnitude in the chewing band 0.77–1.67 Hz) — and a hand-tuned decision tree assigns one of four classes: RESTING, RUMINATING, GRAZING or WALKING. A green LED shows activity, a red LED and a buzzer signal that the animal has been resting for too long, every window is logged as CSV over Serial, and a "time budget" (time per activity) is printed every minute. A 3D-printable case with strap tabs was designed for the wearable version.

# Part 3. Analysis of outcomes

## 3.1 Goals achieved

| Goal | Evidence |
|---|---|
| **G1** Sensor reading | `readMpu()` reads the MPU6050 registers directly with `Wire`, ±2 g range, 10 Hz low-pass filter. Chip ID (WHO_AM_I) is checked at start-up and the sensor is re-configured automatically after an I2C fault. Verified in Wokwi (screenshot in `docs/images/`) and in a PC run of the full firmware (`test/firmware_sim/`) with a simulated sensor that was disconnected for 10 s and then recovered. |
| **G2** Features | Three features in `activity.h`. Intensity removes gravity because \|a\| = 1 g when the animal is still; the angle uses the mean acceleration; rhythm detects regular chewing. |
| **G3** Grazing, rumination, resting | All three are classified, **plus walking**, which is needed so that walking is not confused with grazing. Rumination was not in the midterm version and was added for the final version (commit `Add RUMINATING class…`). |
| **G4** Local low-power decision tree | A 2-level decision tree with 4 thresholds runs on the 8-bit ATmega328P in real time. The classifier needs about 350 bytes of the 2 kB RAM and only a few thousand floating-point operations per 4-second window. No PC, phone or cloud is needed. |
| **G7** Working prototype | Firmware, schematic, Wokwi simulation, PC test and enclosure are complete. *Hardware build: see 3.2.* |

**Classifier test** (`test/test_activity.cpp`): 250 random 4-second windows per class, with random head angle, movement amplitude and frequency in a realistic range and MPU6050-level noise.

| True \\ Predicted | RESTING | RUMINATING | GRAZING | WALKING |
|---|---|---|---|---|
| RESTING | 250 | 0 | 0 | 0 |
| RUMINATING | 0 | 250 | 0 | 0 |
| GRAZING | 0 | 0 | 250 | 0 |
| WALKING | 0 | 0 | 0 | 250 |

Mean features: resting 0.006 g / rhythm 0.20; ruminating 0.022 g / rhythm 0.90; grazing 0.120 g, 53° head down; walking 0.248 g, 5° head down. The classes are clearly separated, so the answer to the research question is **yes in principle**: three simple features and a decision tree are enough, and they fit on the smallest Arduino.

**Important limitation of this evidence:** the signals are synthetic. The 100 % accuracy shows that the algorithm and thresholds work as designed; it does not show the accuracy on a real animal.

## 3.2 Goals not achieved and why

| Goal | Status | Reason |
|---|---|---|
| **G5** Edge Impulse | Not used | Edge Impulse trains a model from **labelled recordings**. No recordings of real animals with known behaviour were available during the course, and a model trained only on synthetic data would just learn the generator. A hand-tuned decision tree was used instead: it answers the same research question, is fully explainable and needs far less memory than a neural network. |
| **G6** LoRa logging | Not done | No LoRa module (e.g. SX1276/RFM95) was available, and a LoRa link also needs a second node or gateway as a receiver. Data is logged over Serial in CSV format, which is the same data a LoRa packet would carry. |
| Validation on a real animal | Not done | No access to an animal with the collar for long enough to record and label behaviour (video + accelerometer). The thresholds were therefore chosen from the physics of the signals (gravity = 1 g, chewing ≈ 1 Hz, grazing = head down) and checked only on synthetic data. |
| Hardware prototype | *[Update after building: e.g. "Built on a breadboard and tested by hand; all four classes and the alarm worked — see photos and video" or describe any problems found.]* | |

## 3.3 Improvements with more time and resources

1. **Real data and tuning.** Fit the collar on a cow for several days, film it, label 4-second windows from the video, and tune the thresholds (or train an Edge Impulse / decision-tree model) on these labels. Report real accuracy, precision and recall per class.
2. **Wireless link.** Replace the Uno with an ESP32 + LoRa module (e.g. Heltec LoRa32) and send the time budget every 15 minutes to a gateway and a simple dashboard; send the alarm immediately.
3. **Battery life.** A 9 V battery powering an Uno lasts only hours. Use a 3.3 V Pro Mini or ESP32 with a Li-ion cell, put the MCU to sleep between samples and use the MPU6050 FIFO and motion-interrupt to wake up — this could reach weeks or months of operation.
4. **Better features.** Add the gyroscope, frequency-domain features (FFT) and longer context (e.g. rumination bouts of 20–60 minutes) to separate rumination from small head movements more reliably.
5. **Per-animal baseline.** Alert on a change from the animal's own normal time budget (e.g. "rumination time down 30 % compared to last week") instead of a fixed inactivity timer.
6. **Robust housing.** Print the case in PETG/ASA, add a gasket for rain, and a curved base that fits the neck.

## 3.4 Conclusion

The project shows that an MPU6050 and a small decision tree running locally on an 8-bit Arduino can separate resting, rumination, grazing and walking from neck movement, and that the result can drive a useful farm alarm and time budget. The core of the proposal (G1–G4) was achieved, including the rumination class that was missing at the midterm. The two parts that depend on resources not available during the course — Edge Impulse training on real labelled data and LoRa transmission — were replaced by a hand-tuned tree and Serial logging, and are the first steps for future work.
