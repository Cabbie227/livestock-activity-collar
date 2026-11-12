// activity.h - activity classification for the Livestock Activity Collar.
//
// Pure C++ with no Arduino dependencies, so the same code runs on the board
// and in the PC test (test/test_activity.cpp).
//
// Collar orientation: the sensor X axis points along the neck towards the
// head, Z points up when the animal stands with its head level.

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <math.h>

enum Activity { RESTING = 0, GRAZING = 1, WALKING = 2 };

// Tunable thresholds
const float ACTIVE_THRESHOLD_G = 0.05f;   // movement intensity above this = animal is moving
const float HEAD_DOWN_ANGLE_DEG = 30.0f;  // head lower than this while moving = grazing

inline const char *activityName(Activity a) {
  switch (a) {
    case RESTING: return "RESTING";
    case GRAZING: return "GRAZING";
    default:      return "WALKING";
  }
}

// Collects accelerometer samples (in g) over one time window and produces
// two features: movement intensity and head-down angle.
class ActivityWindow {
 public:
  ActivityWindow() { reset(); }

  void reset() {
    n = 0;
    sumMag = sumMagSq = 0;
    sumX = sumY = sumZ = 0;
  }

  void addSample(float ax, float ay, float az) {
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    sumMag += mag;
    sumMagSq += mag * mag;
    sumX += ax;
    sumY += ay;
    sumZ += az;
    n++;
  }

  int count() const { return n; }

  // Standard deviation of the acceleration magnitude, in g.
  // Gravity is constant, so this isolates the movement of the animal.
  float intensity() const {
    if (n == 0) return 0;
    float mean = sumMag / n;
    float var = sumMagSq / n - mean * mean;
    return var > 0 ? sqrtf(var) : 0;
  }

  // Angle of the neck below horizontal, in degrees (positive = head down).
  // Uses the mean acceleration, i.e. the direction of gravity.
  float headDownAngle() const {
    if (n == 0) return 0;
    float x = sumX / n, y = sumY / n, z = sumZ / n;
    return atan2f(-x, sqrtf(y * y + z * z)) * 57.29578f;
  }

  Activity classify() const {
    if (intensity() < ACTIVE_THRESHOLD_G) return RESTING;
    if (headDownAngle() > HEAD_DOWN_ANGLE_DEG) return GRAZING;
    return WALKING;
  }

 private:
  int n;
  float sumMag, sumMagSq, sumX, sumY, sumZ;
};

#endif
