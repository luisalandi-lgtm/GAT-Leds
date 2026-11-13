// =====================================================================
// Outputs.h - Uscite analogiche verso il poligrafo
// OUT1 = marker (impulsi a tensione e durata codificate)
// OUT2 = segnale continuo della manovra (angolo, pressione, forza)
// =====================================================================

#ifndef ANSCOVERY_OUTPUTS_H
#define ANSCOVERY_OUTPUTS_H

#include "Config.h"

// Avvia un marker su OUT1. Se un marker e' gia' in corso la richiesta viene ignorata.
void triggerMarker(float voltage, unsigned long durationMs);

// Riporta OUT1 a 0 V quando la durata del marker e' trascorsa (a ogni loop)
void updateMarker();

// Scrive OUT2 in proporzione: 0.0 = DAC 0, 1.0 = DAC 255 (fuori intervallo viene limitato)
void writeOut2Normalized(float normalized);

// Stampa OUT1 e OUT2 su seriale a ogni cambio di OUT1 e ogni OUTPUTS_PRINT_INTERVAL_MS
void printOutputs();

#endif
