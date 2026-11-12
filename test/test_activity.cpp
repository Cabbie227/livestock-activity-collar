// PC test of the activity classifier using synthetic accelerometer signals.
// Build and run:  g++ -O2 -o test_activity test/test_activity.cpp && ./test_activity
//
// The signals are generated, not recorded from an animal. The test checks
// that the algorithm and thresholds behave as designed.

#include <cstdio>
#include <cstdlib>
#include "../firmware/livestock_collar/activity.h"

const float PI_F = 3.14159265f;
const int FS = 20;          // Hz, same as firmware
const int WINDOW = 40;      // samples, same as firmware
const int WINDOWS_PER_CLASS = 200;

float noise(float amplitude) {
  return amplitude * (2.0f * rand() / RAND_MAX - 1.0f);
}

// Generates one sample of the given true activity at time t (seconds).
void synth(Activity truth, float t, float &ax, float &ay, float &az) {
  float pitchDown, move;
  switch (truth) {
    case RESTING:  // lying or standing still, head level, sensor noise only
      pitchDown = 5; move = 0;
      break;
    case GRAZING:  // head down ~50 deg, irregular biting/tearing movements
      pitchDown = 50; move = 0.15f * sinf(2 * PI_F * 1.2f * t) + noise(0.10f);
      break;
    default:       // walking: head level, rhythmic steps ~2 Hz
      pitchDown = 5; move = 0.35f * sinf(2 * PI_F * 2.0f * t) + noise(0.05f);
      break;
  }
  float p = pitchDown * PI_F / 180;
  float g = 1.0f + move;  // movement modulates the measured magnitude
  ax = -sinf(p) * g + noise(0.01f);
  ay = noise(0.01f);
  az = cosf(p) * g + noise(0.01f);
}

int main() {
  srand(42);
  int confusion[3][3] = {{0}};
  double sumIntensity[3] = {0}, sumAngle[3] = {0};

  for (int truth = 0; truth < 3; truth++) {
    for (int w = 0; w < WINDOWS_PER_CLASS; w++) {
      ActivityWindow win;
      for (int i = 0; i < WINDOW; i++) {
        float t = (w * WINDOW + i) / (float)FS;
        float ax, ay, az;
        synth((Activity)truth, t, ax, ay, az);
        win.addSample(ax, ay, az);
      }
      confusion[truth][win.classify()]++;
      sumIntensity[truth] += win.intensity();
      sumAngle[truth] += win.headDownAngle();
    }
  }

  printf("Mean features per true activity (%d windows of 2 s each)\n", WINDOWS_PER_CLASS);
  printf("%-10s %14s %16s\n", "activity", "intensity [g]", "head angle [deg]");
  for (int a = 0; a < 3; a++)
    printf("%-10s %14.3f %16.1f\n", activityName((Activity)a),
           sumIntensity[a] / WINDOWS_PER_CLASS, sumAngle[a] / WINDOWS_PER_CLASS);

  printf("\nConfusion matrix (rows = true, columns = predicted)\n");
  printf("%-10s %9s %9s %9s\n", "", "RESTING", "GRAZING", "WALKING");
  int correct = 0;
  for (int a = 0; a < 3; a++) {
    printf("%-10s %9d %9d %9d\n", activityName((Activity)a),
           confusion[a][0], confusion[a][1], confusion[a][2]);
    correct += confusion[a][a];
  }
  int total = 3 * WINDOWS_PER_CLASS;
  printf("\nAccuracy: %d / %d = %.1f %%\n", correct, total, 100.0 * correct / total);
  return correct == total ? 0 : 1;
}
