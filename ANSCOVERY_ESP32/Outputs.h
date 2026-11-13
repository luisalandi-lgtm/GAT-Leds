// =====================================================================
// Outputs.h - Uscite analogiche verso il poligrafo
// OUT1 = marker (impulsi a tensione e durata codificate)
// OUT2 = segnale continuo della manovra (angolo, pressione, forza, ritmo
//        respiratorio); 0 V in IDLE e nel Cold Face
// =====================================================================

#ifndef ANSCOVERY_OUTPUTS_H
#define ANSCOVERY_OUTPUTS_H

#include "Config.h"

// OUT1 e OUT2 a 0 V: da chiamare per prima cosa all'accensione
void initOutputs();

// Avvia un marker su OUT1. Se un marker e' in corso (o e' appena finito) la richiesta
// va in coda e parte dopo, separata dal precedente da MARKER_GAP_MS a 0 V.
void triggerMarker(float voltage, unsigned long durationMs);

// A ogni loop: chiude il marker scaduto e avvia il successivo in coda
void updateMarker();

// Scrive OUT2 in proporzione: 0.0 = DAC 0, 1.0 = DAC 255 (fuori intervallo viene limitato)
void writeOut2Normalized(float normalized);

// Stampa OUT1 e OUT2 su seriale a ogni cambio di OUT1 e ogni OUTPUTS_PRINT_INTERVAL_MS
void printOutputs();

#endif
