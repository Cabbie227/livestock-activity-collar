// activity.h - activity classification for the Livestock Activity Collar.
//
// Pure C++ with no Arduino dependencies, so the same code runs on the board
// and in the PC test (test/test_activity.cpp).
//
// Collar orientation: the sensor X axis points along the neck towards the
// head, Z points up when the animal stands with its head level.
//
// Classifier = a small, hand-tuned decision tree on three features
// computed over one window of samples:
//
//   intensity < ACTIVE_THRESHOLD_G ?
//   |-- yes: intensity > CHEW_MIN_INTENSITY_G and rhythm > RHYTHM_THRESHOLD ?
//   |        |-- yes: RUMINATING
//   |        `-- no:  RESTING
//   `-- no:  headDownAngle > HEAD_DOWN_ANGLE_DEG ?
//            |-- yes: GRAZING
//            `-- no:  WALKING
//
// Rumination (chewing the cud) is a weak but very regular jaw movement at
// roughly 1 chew per second. It is detected with the autocorrelation of the
// acceleration magnitude at lags that correspond to 0.75-1.6 Hz.

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <math.h>

enum Activity { RESTING = 0, RUMINATING = 1, GRAZING = 2, WALKING = 3 };
const int NUM_ACTIVITIES = 4;

// Sampling and window size (shared by the firmware and the PC test)
const int SAMPLE_RATE_HZ = 20;
const int WINDOW_SAMPLES = 80;  // 80 samples at 20 Hz = 4 s window

// Tunable thresholds
const float ACTIVE_THRESHOLD_G = 0.05f;     // movement intensity above this = animal is moving
const float HEAD_DOWN_ANGLE_DEG = 30.0f;    // head lower than this while moving = grazing
const float CHEW_MIN_INTENSITY_G = 0.012f;  // below this it is only sensor noise
const float RHYTHM_THRESHOLD = 0.5f;        // autocorrelation peak needed for rumination

// Chewing frequency band expressed as autocorrelation lags in samples
const int CHEW_MIN_LAG = 12;  // 12 samples at 20 Hz = 0.6 s period = 1.67 Hz
const int CHEW_MAX_LAG = 26;  // 26 samples at 20 Hz = 1.3 s period = 0.77 Hz

inline const char *activityName(Activity a) {
  switch (a) {
    case RESTING:    return "RESTING";
    case RUMINATING: return "RUMINATING";
    case GRAZING:    return "GRAZING";
    default:         return "WALKING";
  }
}

// Collects accelerometer samples (in g) over one time window and produces
// three features: movement intensity, head-down angle and rhythm.
class ActivityWindow {
 public:
  ActivityWindow() { reset(); }

  void reset() {
    n = 0;
    sumMag = sumMagSq = 0;
    sumX = sumY = sumZ = 0;
  }

  void addSample(float ax, float ay, float az) {
    if (n >= WINDOW_SAMPLES) return;  // window is full
    float m = sqrtf(ax * ax + ay * ay + az * az);
    mag[n] = m;
    sumMag += m;
    sumMagSq += m * m;
    sumX += ax;
    sumY += ay;
    sumZ += az;
    n++;
  }

  int count() const { return n; }
  bool full() const { return n >= WINDOW_SAMPLES; }

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

  // How periodic the movement is in the chewing band, from 0 (random) to
  // about 1 (perfectly regular). Highest normalised autocorrelation of the
  // magnitude over lags CHEW_MIN_LAG..CHEW_MAX_LAG.
  float rhythm() const {
    if (n <= CHEW_MAX_LAG) return 0;
    float mean = sumMag / n;
    float energy = 0;
    for (int i = 0; i < n; i++) energy += (mag[i] - mean) * (mag[i] - mean);
    if (energy <= 0) return 0;

    float best = 0;
    for (int lag = CHEW_MIN_LAG; lag <= CHEW_MAX_LAG; lag++) {
      float s = 0;
      for (int i = 0; i + lag < n; i++) s += (mag[i] - mean) * (mag[i + lag] - mean);
      // Scale by n / (n - lag) so long lags are not penalised for having fewer terms
      float r = (s / energy) * n / (n - lag);
      if (r > best) best = r;
    }
    return best;
  }

  Activity classify() const {
    float in = intensity();
    if (in < ACTIVE_THRESHOLD_G) {
      if (in > CHEW_MIN_INTENSITY_G && rhythm() > RHYTHM_THRESHOLD) return RUMINATING;
      return RESTING;
    }
    if (headDownAngle() > HEAD_DOWN_ANGLE_DEG) return GRAZING;
    return WALKING;
  }

 private:
  int n;
  float mag[WINDOW_SAMPLES];  // 80 floats = 320 bytes of the Uno's 2 kB RAM
  float sumMag, sumMagSq, sumX, sumY, sumZ;
};

#endif
