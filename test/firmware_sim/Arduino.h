#pragma once
#include <cstdint>
#include <cstdio>
#include <cmath>
#define F(x) (x)
#define OUTPUT 1
#define HEX 16
typedef uint8_t byte;
unsigned long millis();
void pinMode(int,int); void digitalWrite(int,int); int digitalRead(int); void tone(int,int,int);
struct SerialT {
  void begin(long){}
  void print(const char*s){printf("%s",s);} void print(char c){printf("%c",c);}
  void print(unsigned long v){printf("%lu",v);} void print(int v){printf("%d",v);}
  void print(int v,int b){printf(b==16?"%X":"%d",v);}
  void print(double v,int d){printf("%.*f",d,v);}
  void println(){printf("\n");} void println(const char*s){printf("%s\n",s);}
  void println(int v){printf("%d\n",v);} void println(int v,int b){printf(b==16?"%X\n":"%d\n",v);}
};
extern SerialT Serial;
struct WireT {
  void begin(); void beginTransmission(uint8_t); void write(uint8_t);
  uint8_t endTransmission(bool stop=true); uint8_t requestFrom(uint8_t,uint8_t); int read();
};
extern WireT Wire;
