// =====================================================================
// Display.cpp - Due display TM1637 a 4 cifre con i due punti centrali
// Display 1 = Tempo 1 (test), display 2 = Tempo 2 (fase della manovra).
// =====================================================================

#include <Arduino.h>
#include <TM1637Display.h>
#include "Display.h"

// Bit dei due punti centrali del display
#define DISPLAY_COLON 0b01000000

static TM1637Display displayT1(PIN_TM1637_CLK1, PIN_TM1637_DIO1);
static TM1637Display displayT2(PIN_TM1637_CLK2, PIN_TM1637_DIO2);

void initDisplays() {
    displayT1.setBrightness(DISPLAY_BRIGHTNESS);
    displayT2.setBrightness(DISPLAY_BRIGHTNESS);

    displayT1.showNumberDecEx(0, DISPLAY_COLON, true);
    displayT2.showNumberDecEx(0, DISPLAY_COLON, true);
}

// Secondi -> MM:SS (es. 125 s -> 02:05), con zeri iniziali e due punti
static void showTime(TM1637Display &display, uint32_t seconds) {
    uint32_t mins = seconds / 60;
    uint32_t secs = seconds % 60;
    display.showNumberDecEx((mins * 100) + secs, DISPLAY_COLON, true);
}

// Riscrive i display a ogni chiamata, anche se il valore non cambia
void updateDisplays(uint32_t t1Seconds, uint32_t t2Seconds) {
    showTime(displayT1, t1Seconds);
    showTime(displayT2, t2Seconds);
}
