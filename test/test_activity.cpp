// PC test of the activity classifier using synthetic accelerometer signals.
// Build and run:  g++ -O2 -o test_activity test/test_activity.cpp && ./test_activity
//
// The signals are generated, not recorded from an animal. The test checks
// that the algorithm and thresholds behave as designed. To make it less
// trivial, every window gets a random head angle, movement amplitude and
// movement frequency inside a realistic range for its activity.

#include <cstdio>
#include <cstdlib>
#include "../firmware/livestock_collar/activity.h"

const float PI_F = 3.14159265f;
const int WINDOWS_PER_CLASS = 250;
const float MIN_ACCURACY = 95.0f;  // test fails below this

float noise(float amplitude) {
  return amplitude * (2.0f * rand() / RAND_MAX - 1.0f);
}

float uniform(float lo, float hi) {
  return lo + (hi - lo) * rand() / (float)RAND_MAX;
}

// Random parameters of one window of a given activity.
struct Params {
  float pitchDown;  // head-down angle, degrees
  float amp;        // amplitude of the rhythmic movement, g
  float freq;       // frequency of the rhythmic movement, Hz
  float jitter;     // irregular movement, g
};

Params randomParams(Activity truth) {
  Params p;
  switch (truth) {
    case RESTING:     // lying or standing still, sensor noise only
      p = {uniform(-5, 20), 0, 0, 0};
      break;
    case RUMINATING:  // head level or slightly down, weak regular jaw movement ~1 Hz
      p = {uniform(0, 20), uniform(0.02f, 0.04f), uniform(0.8f, 1.5f), 0.005f};
      break;
    case GRAZING:     // head down, biting/tearing, partly irregular
      p = {uniform(40, 65), uniform(0.10f, 0.20f), uniform(0.8f, 1.5f), 0.10f};
      break;
    default:          // walking: head level, rhythmic steps ~1.5-2.5 Hz
      p = {uniform(-5, 15), uniform(0.25f, 0.45f), uniform(1.5f, 2.5f), 0.05f};
      break;
  }
  return p;
}

// One accelerometer sample at time t (seconds).
void synth(const Params &p, float t, float phase, float &ax, float &ay, float &az) {
  float move = p.amp * sinf(2 * PI_F * p.freq * t + phase) + noise(p.jitter);
  float pitch = p.pitchDown * PI_F / 180;
  float g = 1.0f + move;                 // movement modulates the measured magnitude
  ax = -sinf(pitch) * g + noise(0.01f);  // 0.01 g = MPU6050 noise level
  ay = noise(0.01f);
  az = cosf(pitch) * g + noise(0.01f);
}

int main() {
  srand(42);
  int confusion[NUM_ACTIVITIES][NUM_ACTIVITIES] = {{0}};
  double sumIn[NUM_ACTIVITIES] = {0}, sumAngle[NUM_ACTIVITIES] = {0}, sumRhythm[NUM_ACTIVITIES] = {0};

  for (int truth = 0; truth < NUM_ACTIVITIES; truth++) {
    for (int w = 0; w < WINDOWS_PER_CLASS; w++) {
      Params p = randomParams((Activity)truth);
      float phase = uniform(0, 2 * PI_F);
      ActivityWindow win;
      for (int i = 0; i < WINDOW_SAMPLES; i++) {
        float ax, ay, az;
        synth(p, i / (float)SAMPLE_RATE_HZ, phase, ax, ay, az);
        win.addSample(ax, ay, az);
      }
      confusion[truth][win.classify()]++;
      sumIn[truth] += win.intensity();
      sumAngle[truth] += win.headDownAngle();
      sumRhythm[truth] += win.rhythm();
    }
  }

  printf("Mean features per true activity (%d random windows of %d s each)\n",
         WINDOWS_PER_CLASS, WINDOW_SAMPLES / SAMPLE_RATE_HZ);
  printf("%-11s %14s %17s %8s\n", "activity", "intensity [g]", "head angle [deg]", "rhythm");
  for (int a = 0; a < NUM_ACTIVITIES; a++)
    printf("%-11s %14.3f %17.1f %8.2f\n", activityName((Activity)a),
           sumIn[a] / WINDOWS_PER_CLASS, sumAngle[a] / WINDOWS_PER_CLASS,
           sumRhythm[a] / WINDOWS_PER_CLASS);

  printf("\nConfusion matrix (rows = true, columns = predicted)\n");
  printf("%-11s", "");
  for (int a = 0; a < NUM_ACTIVITIES; a++) printf(" %10s", activityName((Activity)a));
  printf("\n");
  int correct = 0;
  for (int t = 0; t < NUM_ACTIVITIES; t++) {
    printf("%-11s", activityName((Activity)t));
    for (int a = 0; a < NUM_ACTIVITIES; a++) printf(" %10d", confusion[t][a]);
    printf("\n");
    correct += confusion[t][t];
  }
  int total = NUM_ACTIVITIES * WINDOWS_PER_CLASS;
  float accuracy = 100.0f * correct / total;
  printf("\nAccuracy: %d / %d = %.1f %%\n", correct, total, accuracy);
  return accuracy >= MIN_ACCURACY ? 0 : 1;
}
