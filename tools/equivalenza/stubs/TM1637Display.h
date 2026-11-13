#pragma once
#include <Arduino.h>
class TM1637Display {
public:
    TM1637Display(uint8_t clk, uint8_t dio) : clk_(clk) {}
    void setBrightness(uint8_t b, bool on = true);
    void showNumberDecEx(int num, uint8_t dots = 0, bool leading_zero = false, uint8_t length = 4, uint8_t pos = 0);
private:
    uint8_t clk_;
};
