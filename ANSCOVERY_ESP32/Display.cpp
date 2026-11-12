#include <Arduino.h>
#include <TM1637Display.h>
#include "Display.h"

static TM1637Display displayT1(PIN_TM1637_CLK1, PIN_TM1637_DIO1);
static TM1637Display displayT2(PIN_TM1637_CLK2, PIN_TM1637_DIO2);

static const uint8_t COLON = 0b01000000;
static const uint32_t MAX_SECONDS = 99UL * 60 + 59; // 99:59

static uint32_t shownT1 = UINT32_MAX;
static uint32_t shownT2 = UINT32_MAX;

static void showTime(TM1637Display &display, uint32_t seconds) {
    if (seconds > MAX_SECONDS) seconds = MAX_SECONDS;
    uint32_t mins = seconds / 60;
    uint32_t secs = seconds % 60;
    display.showNumberDecEx((int)(mins * 100 + secs), COLON, true);
}

void initDisplayHardware() {
    displayT1.setBrightness(0x0f);
    displayT2.setBrightness(0x0f);
    updateDisplays(0, 0);
}

void updateDisplays(uint32_t t1Seconds, uint32_t t2Seconds) {
    if (t1Seconds != shownT1) {
        showTime(displayT1, t1Seconds);
        shownT1 = t1Seconds;
    }
    if (t2Seconds != shownT2) {
        showTime(displayT2, t2Seconds);
        shownT2 = t2Seconds;
    }
}
