#pragma once
#include <Arduino.h>
struct TwoWire{ void begin(int,int){} void setClock(int){} void beginTransmission(int){} int write(int){return 1;} int endTransmission(bool=true); int requestFrom(uint8_t,uint8_t); int read(); };
extern TwoWire Wire;
