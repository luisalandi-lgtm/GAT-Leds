#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 2
#define ADC_11db 3
extern uint32_t g_now;
inline uint32_t millis(){return g_now;}
void pinMode(int,int); int digitalRead(int); int analogRead(int);
void analogReadResolution(int); void analogSetPinAttenuation(int,int);
void dacWrite(int,int);
struct SerialC { void begin(int){} int printf(const char*f,...){va_list a;va_start(a,f);printf_t(f,a);va_end(a);return 0;} void printf_t(const char*,va_list); };
extern SerialC Serial;
