// Runs the real Arduino sketch on a PC with a simulated MPU6050.
// Arduino.h / Wire.h here are minimal stand-ins for the Arduino core.
//
// Scenario (simulated time): 0-20 s walking, 20-40 s ruminating,
// 40-80 s resting (alarm after 30 s), 80-90 s sensor disconnected,
// 90-130 s grazing. The sketch output is printed to the console.
//
// Build and run from the repository root:
//   cp firmware/livestock_collar/livestock_collar.ino /tmp/sketch.cpp
//   g++ -O1 -include test/firmware_sim/Arduino.h -Itest/firmware_sim \
//       -Ifirmware/livestock_collar test/firmware_sim/sim_main.cpp /tmp/sketch.cpp -o firmware_sim
//   ./firmware_sim
#include "Arduino.h"
#include <cstdlib>
SerialT Serial; WireT Wire;
static unsigned long T=0; unsigned long millis(){return T;}
int pins[20]; int buzz=0;
void pinMode(int,int){} void digitalWrite(int p,int v){pins[p]=v;} int digitalRead(int p){return pins[p];} void tone(int,int,int){buzz++;}
static uint8_t reg; static int idx; static uint8_t out[8]; static bool sensorOk=true;
float nz(float a){return a*(2.0f*rand()/RAND_MAX-1);}
// scenario: 0-20s walking, 20-40s ruminating, 40-80s resting, 80-90s sensor unplugged, 90-130s grazing
void fill(){
  float t=T/1000.0f, pitch=5, move=0;
  if(t<20){move=0.35f*sinf(2*3.14159f*2*t)+nz(0.05f);}
  else if(t<40){move=0.03f*sinf(2*3.14159f*1.1f*t);pitch=10;}
  else if(t<80){}
  else {pitch=50;move=0.15f*sinf(2*3.14159f*1.2f*t)+nz(0.1f);}
  float p=pitch*3.14159f/180, g=1+move;
  int16_t v[4]={(int16_t)((-sinf(p)*g+nz(0.01f))*16384),(int16_t)(nz(0.01f)*16384),(int16_t)((cosf(p)*g+nz(0.01f))*16384),(int16_t)((25-36.53)*340)};
  for(int i=0;i<4;i++){out[2*i]=v[i]>>8;out[2*i+1]=v[i]&0xFF;}
}
void WireT::begin(){} void WireT::beginTransmission(uint8_t){idx=-1;}
void WireT::write(uint8_t b){if(idx<0){reg=b;idx=0;}}
uint8_t WireT::endTransmission(bool){return sensorOk?0:2;}
uint8_t WireT::requestFrom(uint8_t,uint8_t n){if(!sensorOk)return 0; if(reg==0x3B)fill(); idx=0; return n;}
int WireT::read(){ if(reg==0x75) return 0x68; return out[idx++]; }
void setup(); void loop();
int main(){ setup(); for(T=0;T<130000;T+=10){ sensorOk=!(T>=80000&&T<90000); loop(); } fprintf(stderr,"beeps=%d\n",buzz); }
