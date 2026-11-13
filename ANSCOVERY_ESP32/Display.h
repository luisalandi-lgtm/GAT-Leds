// =====================================================================
// Display.h - Display TM1637 di Tempo 1 e Tempo 2
// =====================================================================

#ifndef DISPLAY_H
#define DISPLAY_H

#include "Config.h"

// Luminosita' e 00:00 su entrambi i display
void initDisplays();

// Mostra i due tempi (in secondi) nel formato MM:SS
void updateDisplays(uint32_t t1Seconds, uint32_t t2Seconds);

#endif
