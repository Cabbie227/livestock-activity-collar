// Livestock Activity Collar
// ELT.123 Electronic Components and Circuits - Arduino project
//
// Reads an MPU6050 accelerometer worn on the animal's neck, classifies the
// activity every 2 seconds (RESTING / GRAZING / WALKING), logs it over Serial
// as CSV and raises an alarm if the animal has not moved for too long.
//
// Wiring (Arduino Uno / Nano):
//   MPU6050 VCC -> 5V      MPU6050 GND -> GND
//   MPU6050 SDA -> A4      MPU6050 SCL -> A5
//   Green LED   -> D5 (through 220 ohm) - blinks when the animal is active
//   Red LED     -> D6 (through 220 ohm) - inactivity alarm
//   Buzzer      -> D8                   - inactivity alarm

#include <Wire.h>
#include "activity.h"

const uint8_t MPU_ADDR = 0x68;
const uint8_t PIN_LED_ACTIVE = 5;
const uint8_t PIN_LED_ALERT = 6;
const uint8_t PIN_BUZZER = 8;

const unsigned long SAMPLE_PERIOD_MS = 50;   // 20 Hz sampling
const int SAMPLES_PER_WINDOW = 40;           // 40 samples = 2 s window

// Alarm if the animal rests without a break for this long.
// 30 s is for demonstration; on a real animal use several hours.
const unsigned long INACTIVITY_ALERT_MS = 30000UL;

ActivityWindow window;
unsigned long lastSampleMs = 0;
unsigned long lastActiveMs = 0;
unsigned long secondsIn[3] = {0, 0, 0};  // total time per activity

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

// Reads acceleration in g and chip temperature in degrees C.
bool readMpu(float &ax, float &ay, float &az, float &tempC) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, (uint8_t)8) != 8) return false;

  int16_t rawX = (Wire.read() << 8) | Wire.read();
  int16_t rawY = (Wire.read() << 8) | Wire.read();
  int16_t rawZ = (Wire.read() << 8) | Wire.read();
  int16_t rawT = (Wire.read() << 8) | Wire.read();

  // +/-2 g range: 16384 LSB per g
  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;
  tempC = rawT / 340.0 + 36.53;  // formula from the MPU6050 datasheet
  return true;
}

void setup() {
  pinMode(PIN_LED_ACTIVE, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
  writeRegister(0x6B, 0x00);  // PWR_MGMT_1: wake the sensor up
  writeRegister(0x1C, 0x00);  // ACCEL_CONFIG: +/-2 g
  writeRegister(0x1A, 0x04);  // CONFIG: 20 Hz low-pass filter

  Serial.println(F("time_s,activity,intensity_g,head_angle_deg,temp_c,alert"));
  lastActiveMs = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs < SAMPLE_PERIOD_MS) return;
  lastSampleMs = now;

  float ax, ay, az, tempC;
  if (!readMpu(ax, ay, az, tempC)) {
    Serial.println(F("ERROR: MPU6050 not responding"));
    digitalWrite(PIN_LED_ALERT, !digitalRead(PIN_LED_ALERT));  // fast blink = sensor fault
    return;
  }
  window.addSample(ax, ay, az);
  if (window.count() < SAMPLES_PER_WINDOW) return;

  // One full window collected: classify and report.
  Activity activity = window.classify();
  secondsIn[activity] += SAMPLE_PERIOD_MS * SAMPLES_PER_WINDOW / 1000;

  if (activity != RESTING) lastActiveMs = now;
  bool alert = (now - lastActiveMs) >= INACTIVITY_ALERT_MS;

  digitalWrite(PIN_LED_ACTIVE, activity != RESTING);
  digitalWrite(PIN_LED_ALERT, alert);
  if (alert) tone(PIN_BUZZER, 2000, 200);

  Serial.print(now / 1000);
  Serial.print(',');
  Serial.print(activityName(activity));
  Serial.print(',');
  Serial.print(window.intensity(), 3);
  Serial.print(',');
  Serial.print(window.headDownAngle(), 1);
  Serial.print(',');
  Serial.print(tempC, 1);
  Serial.print(',');
  Serial.println(alert ? 1 : 0);

  window.reset();
}
