// Livestock Activity Collar
// ELT.123 Electronic Components and Circuits - Arduino project
// Author: Nazar Smatov
//
// Reads an MPU6050 accelerometer worn on the animal's neck, classifies the
// activity every 4 seconds (RESTING / RUMINATING / GRAZING / WALKING), logs it
// over Serial as CSV and raises an alarm if the animal has not moved for too
// long. Once a minute it also prints how the time was split between the
// activities (a "time budget"), which is what a farmer actually looks at.
//
// Wiring (Arduino Uno / Nano):
//   MPU6050 VCC -> 5V      MPU6050 GND -> GND
//   MPU6050 SDA -> A4      MPU6050 SCL -> A5
//   Green LED   -> D5 (through 220 ohm) - on when the animal is active
//   Red LED     -> D6 (through 220 ohm) - inactivity alarm / sensor fault
//   Buzzer      -> D8                   - inactivity alarm

#include <Wire.h>
#include "activity.h"

// ---------- Pins and I2C address ----------
const uint8_t MPU_ADDR = 0x68;  // AD0 pin low
const uint8_t PIN_LED_ACTIVE = 5;
const uint8_t PIN_LED_ALERT = 6;
const uint8_t PIN_BUZZER = 8;

// ---------- MPU6050 registers (from the register map) ----------
const uint8_t REG_CONFIG = 0x1A;        // digital low-pass filter
const uint8_t REG_ACCEL_CONFIG = 0x1C;  // accelerometer range
const uint8_t REG_ACCEL_XOUT_H = 0x3B;  // first of 8 bytes: accel X, Y, Z, temperature
const uint8_t REG_PWR_MGMT_1 = 0x6B;    // sleep / clock source
const uint8_t REG_WHO_AM_I = 0x75;      // chip ID, 0x68 for a genuine MPU6050

// ---------- Timing ----------
const unsigned long SAMPLE_PERIOD_MS = 1000UL / SAMPLE_RATE_HZ;          // 50 ms = 20 Hz
const unsigned long WINDOW_MS = SAMPLE_PERIOD_MS * WINDOW_SAMPLES;      // 4000 ms
const unsigned long SUMMARY_PERIOD_MS = 60000UL;                        // time budget every minute
const unsigned long ERROR_PRINT_PERIOD_MS = 1000UL;                     // limit error messages

// Alarm if the animal rests without a break for this long.
// 30 s is for demonstration; on a real animal use several hours.
const unsigned long INACTIVITY_ALERT_MS = 30000UL;

// ---------- State ----------
ActivityWindow window;
unsigned long lastSampleMs = 0;
unsigned long lastActiveMs = 0;
unsigned long lastSummaryMs = 0;
unsigned long lastErrorPrintMs = 0;
unsigned long msIn[NUM_ACTIVITIES] = {0, 0, 0, 0};  // total time per activity

// ---------- MPU6050 helpers ----------
bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Returns the register value, or -1 if the sensor does not answer.
int readRegister(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return -1;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)1) != 1) return -1;
  return Wire.read();
}

// Writes the configuration registers. Returns false if the sensor does not answer.
bool configureMpu() {
  return writeRegister(REG_PWR_MGMT_1, 0x00) &&    // wake up, internal 8 MHz clock
         writeRegister(REG_ACCEL_CONFIG, 0x00) &&  // +/-2 g full scale
         writeRegister(REG_CONFIG, 0x05);          // low-pass filter ~10 Hz (= half the 20 Hz sample rate)
}

// Checks the chip ID, then configures the sensor. Returns false if it is not found.
bool initMpu() {
  int id = readRegister(REG_WHO_AM_I);
  if (id < 0) {
    Serial.println(F("# ERROR: MPU6050 not found - check SDA/SCL wiring"));
    return false;
  }
  if (id != 0x68) {
    // Many cheap GY-521 boards carry clones (MPU6500 etc.) that still work.
    Serial.print(F("# WARNING: unexpected WHO_AM_I = 0x"));
    Serial.println(id, HEX);
  }
  return configureMpu();
}

// Reads acceleration in g and chip temperature in degrees C.
bool readMpu(float &ax, float &ay, float &az, float &tempC) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)8) != 8) return false;

  // Each value is 16-bit, high byte first.
  uint8_t buf[8];
  for (uint8_t i = 0; i < 8; i++) buf[i] = Wire.read();
  int16_t rawX = (int16_t)((buf[0] << 8) | buf[1]);
  int16_t rawY = (int16_t)((buf[2] << 8) | buf[3]);
  int16_t rawZ = (int16_t)((buf[4] << 8) | buf[5]);
  int16_t rawT = (int16_t)((buf[6] << 8) | buf[7]);

  // +/-2 g range: 16384 LSB per g
  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;
  tempC = rawT / 340.0 + 36.53;  // formula from the MPU6050 datasheet
  return true;
}

// ---------- Reporting ----------
// Prints the time budget as a comment line (starts with '#') so that the CSV
// log can still be opened directly in Excel or Python.
void printSummary() {
  unsigned long total = 0;
  for (int a = 0; a < NUM_ACTIVITIES; a++) total += msIn[a];
  if (total == 0) return;

  Serial.print(F("# time budget:"));
  for (int a = 0; a < NUM_ACTIVITIES; a++) {
    Serial.print(' ');
    Serial.print(activityName((Activity)a));
    Serial.print(' ');
    Serial.print(msIn[a] / 1000);
    Serial.print(F(" s ("));
    Serial.print(100UL * msIn[a] / total);
    Serial.print(F("%)"));
  }
  Serial.println();
}

void handleSensorError(unsigned long now) {
  digitalWrite(PIN_LED_ALERT, !digitalRead(PIN_LED_ALERT));  // fast blink = sensor fault
  if (now - lastErrorPrintMs >= ERROR_PRINT_PERIOD_MS) {
    lastErrorPrintMs = now;
    Serial.println(F("# ERROR: MPU6050 not responding"));
    configureMpu();  // re-configure, e.g. after a loose wire is fixed or the sensor reset
  }
  window.reset();  // do not mix samples from before and after the fault
}

// ---------- Main program ----------
void setup() {
  pinMode(PIN_LED_ACTIVE, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
  initMpu();

  Serial.println(F("time_s,activity,intensity_g,head_angle_deg,rhythm,temp_c,alert"));
  lastActiveMs = millis();
  lastSummaryMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_PERIOD_MS) return;
  lastSampleMs = now;

  // 1. Read one sample (20 times per second)
  float ax, ay, az, tempC;
  if (!readMpu(ax, ay, az, tempC)) {
    handleSensorError(now);
    return;
  }
  window.addSample(ax, ay, az);
  if (!window.full()) return;

  // 2. One full window collected: classify it
  Activity activity = window.classify();
  msIn[activity] += WINDOW_MS;

  // 3. Inactivity alarm. Rumination counts as normal activity: a cow that
  //    chews the cud is healthy, one that stops ruminating is often sick.
  if (activity != RESTING) lastActiveMs = now;
  bool alert = (now - lastActiveMs) >= INACTIVITY_ALERT_MS;

  digitalWrite(PIN_LED_ACTIVE, activity != RESTING);
  digitalWrite(PIN_LED_ALERT, alert);
  if (alert) tone(PIN_BUZZER, 2000, 200);  // 2 kHz beep, 200 ms

  // 4. Log one CSV line
  Serial.print(now / 1000);
  Serial.print(',');
  Serial.print(activityName(activity));
  Serial.print(',');
  Serial.print(window.intensity(), 3);
  Serial.print(',');
  Serial.print(window.headDownAngle(), 1);
  Serial.print(',');
  Serial.print(window.rhythm(), 2);
  Serial.print(',');
  Serial.print(tempC, 1);
  Serial.print(',');
  Serial.println(alert ? 1 : 0);

  // 5. Time budget once a minute
  if (now - lastSummaryMs >= SUMMARY_PERIOD_MS) {
    lastSummaryMs = now;
    printSummary();
  }

  window.reset();
}
