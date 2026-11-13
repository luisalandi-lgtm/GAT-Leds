// Stub minimale di Arduino per la verifica di equivalenza su PC.
// Ogni chiamata con effetto osservabile viene registrata nella traccia.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>
using std::abs;

#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define DEC 10
enum adc_attenuation_t { ADC_0db, ADC_2_5db, ADC_6db, ADC_11db };
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void traceEvent(const std::string &s);
unsigned long millis();
void delay(uint32_t ms);
void pinMode(uint8_t pin, uint8_t mode);
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t val);
uint16_t analogRead(uint8_t pin);
void analogReadResolution(uint8_t bits);
void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t att);
void dacWrite(uint8_t pin, uint8_t value);
long map(long x, long in_min, long in_max, long out_min, long out_max);

class HardwareSerial {
public:
    void begin(unsigned long baud);
    size_t write(const char *s);
    size_t print(const char *s) { return write(s); }
    size_t print(char c) { char b[2] = {c, 0}; return write(b); }
    size_t print(unsigned char n, int base = DEC) { return print((unsigned long)n, base); }
    size_t print(int n, int base = DEC) { return print((long)n, base); }
    size_t print(unsigned int n, int base = DEC) { return print((unsigned long)n, base); }
    size_t print(long n, int base = DEC);
    size_t print(unsigned long n, int base = DEC);
    size_t print(double n, int digits = 2);
    size_t println() { return write("\r\n"); }
    template <typename T> size_t println(T v) { size_t n = print(v); return n + println(); }
    template <typename T> size_t println(T v, int f) { size_t n = print(v, f); return n + println(); }
};
extern HardwareSerial Serial;
