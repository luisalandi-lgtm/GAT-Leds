#pragma once
#include <Arduino.h>
#define NEO_GRB 0x52
#define NEO_KHZ800 0x0000
class Adafruit_NeoPixel {
public:
    Adafruit_NeoPixel(uint16_t n, int16_t pin, uint16_t type) : n_(n), pin_(pin) { memset(px_, 0, sizeof(px_)); }
    void begin() {}
    void setBrightness(uint8_t b) { bright_ = b; }
    void clear() { memset(px_, 0, sizeof(px_)); }
    void show();
    void fill(uint32_t c) { for (int i = 0; i < n_; i++) setPixelColor(i, c); }
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }
    void setPixelColor(uint16_t i, uint32_t c) { if (i < n_) px_[i] = c; }
    uint16_t numPixels() const { return n_; }
private:
    uint16_t n_; int16_t pin_; uint8_t bright_ = 255; uint32_t px_[256];
};
