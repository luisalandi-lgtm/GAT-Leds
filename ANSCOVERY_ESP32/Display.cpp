// =====================================================================
// Display.cpp - Due display TM1637 a 4 cifre con i due punti centrali
// Display 1 = Tempo 1 (test), display 2 = Tempo 2 (fase della manovra).
// =====================================================================

#include <Arduino.h>
#include <TM1637Display.h>
#include "Display.h"

// Pin corretti definiti nel progetto
static TM1637Display displayT1(PIN_TM1637_CLK1, PIN_TM1637_DIO1);
static TM1637Display displayT2(PIN_TM1637_CLK2, PIN_TM1637_DIO2);

// Usate solo da setDisplayTime(), che non viene chiamata
static uint32_t t1Seconds = 0;
static uint32_t t2Seconds = 0;

// Luminosita' massima e 00:00 su entrambi i display
void initDisplayHardware() {
    displayT1.setBrightness(0x0f);
    displayT2.setBrightness(0x0f);
    
    // Inizializza mostrato 00:00 con i due punti attivi
    displayT1.showNumberDecEx(0, 0b01000000, true);
    displayT2.showNumberDecEx(0, 0b01000000, true);
}

// Non chiamata nel codice attuale
void setDisplayTime(uint32_t t1Sec, uint32_t t2Sec) {
    t1Seconds = t1Sec;
    t2Seconds = t2Sec;
}

// Non chiamata nel codice attuale: updateDisplays() fa la stessa conversione
static void showTimeOnDisplay(TM1637Display &display, uint32_t seconds) {
    uint32_t mins = seconds / 60;
    uint32_t secs = seconds % 60;
    uint16_t formattedValue = (mins * 100) + secs;
    
    // 0b01000000 attiva i due punti del display (:
    display.showNumberDecEx(formattedValue, 0b01000000, true);
}

// Converte i secondi in MM:SS (es. 125 s -> 02:05) e riscrive i display.
// Viene chiamata a ogni loop e riscrive sempre, anche se il valore non cambia.
void updateDisplays(uint32_t t1Seconds, uint32_t t2Seconds) {
    uint32_t min1 = t1Seconds / 60;
    uint32_t sec1 = t1Seconds % 60;
    displayT1.showNumberDecEx((min1 * 100) + sec1, 0b01000000, true);

    uint32_t min2 = t2Seconds / 60;
    uint32_t sec2 = t2Seconds % 60;
    displayT2.showNumberDecEx((min2 * 100) + sec2, 0b01000000, true);
}