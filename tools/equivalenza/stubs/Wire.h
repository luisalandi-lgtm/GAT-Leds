#pragma once
#include <Arduino.h>
class TwoWire {
public:
    bool begin(int sda, int scl);
    void beginTransmission(int addr);
    size_t write(uint8_t b);
    uint8_t endTransmission(bool stop = true);
    uint8_t requestFrom(int addr, int qty, int stop);
    int available();
    int read();
};
extern TwoWire Wire;
