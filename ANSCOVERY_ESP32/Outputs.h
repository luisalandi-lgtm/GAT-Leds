#ifndef OUTPUTS_H
#define OUTPUTS_H

#include "Config.h"

// Gestione delle uscite analogiche verso il poligrafo.
// Tutte le tensioni sono "logiche" 0..OUT_FULL_SCALE_V (vedi Config.h).

void initOutputs();

// Accoda un impulso marker su OUT1 (i marker non vengono mai scartati)
void queueMarker(float volts, uint32_t durationMs);

// Valore del segnale continuo su OUT2
void setOut2(float volts);

// Da chiamare a ogni loop: avanza la coda marker e scrive i DAC
void updateOutputs();

float getOut1Volts();
float getOut2Volts();

#endif
