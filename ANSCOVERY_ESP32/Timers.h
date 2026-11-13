// =====================================================================
// Timers.h - Tempo 1 (durata del test) e Tempo 2 (durata della fase)
// Ogni tempo o sta contando (dal suo istante di partenza) o e' fermo e
// mostra un valore "congelato".
// =====================================================================

#ifndef TIMERS_H
#define TIMERS_H

#include "Config.h"

// Entrambi i tempi fermi a zero
void initTimers();

// Pulsante AZZERA T1: se T1 e' fermo riparte da 0, se sta contando si ferma
void toggleTempo1();

// T2 riparte da 0. Il valore congelato precedente resta memorizzato.
void restartTempo2();

// T2 riparte da 0 e il valore congelato viene azzerato
void restartTempo2ClearFrozen();

// T2 si ferma e torna a mostrare l'ultimo valore congelato
void stopTempo2();

// T2 si ferma mostrando il tempo trascorso meno excludedMs
// (per escludere l'attesa finale che ha confermato la fine della fase)
void freezeTempo2Excluding(uint32_t excludedMs);

// Millisecondi dalla partenza di T2
uint32_t tempo2ElapsedMs();

// Valori in secondi da mostrare: tempo corrente se sta contando, altrimenti quello congelato
float getTempo1Seconds();
float getTempo2Seconds();

#endif
