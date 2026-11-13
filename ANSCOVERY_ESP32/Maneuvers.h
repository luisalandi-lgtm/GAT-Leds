// =====================================================================
// Maneuvers.h - Funzioni delle singole manovre
// Ogni manovra ha il suo file .cpp con la sua macchina a stati interna.
// Le funzioni update*() vengono chiamate da FSM.cpp a ogni loop mentre
// la manovra e' attiva; a fine manovra chiamano setTestMode(TEST_MODE_IDLE).
// =====================================================================

#ifndef ANSCOVERY_MANEUVERS_H
#define ANSCOVERY_MANEUVERS_H

#include "Config.h"

// ---------- Tilt Test (TiltTest.cpp) ----------
void resetTiltPhase();          // torna alla fase iniziale (attesa della salita)
void updateTiltTest();

// ---------- Valsalva (Valsalva.cpp) ----------
void resetValsalvaPhase();      // torna alla fase iniziale (attesa dei 25 mmHg)
void updateValsalva();

// ---------- Hand Grip (Handgrip.cpp) ----------
void resetHandgripCalib();      // ricomincia la taratura (stampa un messaggio)
void updateHandgrip();
void refreshHandgripLeds();     // barra LED, solo dopo la taratura
float getHandgripTarget();      // target in kg (0 prima della taratura)

// ---------- Deep Breathing (DeepBreathing.cpp) ----------
void resetDeepBreathing();      // barra e registrazione disattivate
void startDeepBreathing();      // doppio click in IDLE
void deepBreathingOnClick();    // click singolo: avvia la registrazione
void deepBreathingOnDoubleClick(); // doppio click: stop manuale
void updateDeepBreathing();
bool isDeepBreathingLedsActive();

#endif
