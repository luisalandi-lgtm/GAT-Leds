#ifndef DISPLAY_H
#define DISPLAY_H

#include "Config.h"

void initDisplayHardware();
// Aggiorna i display TM1637 (MM:SS) solo quando il valore cambia
void updateDisplays(uint32_t t1Seconds, uint32_t t2Seconds);

#endif
