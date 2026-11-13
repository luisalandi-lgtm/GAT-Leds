// =====================================================================
// FSM.h - Macchina a stati del sistema
// =====================================================================

#ifndef ANSCOVERY_FSM_H
#define ANSCOVERY_FSM_H

#include "Config.h"

// Manovra attiva
enum SystemTestMode {
    TEST_MODE_IDLE,
    TEST_MODE_TILT,
    TEST_MODE_DEEP_BREATHING,
    TEST_MODE_VALSALVA,
    TEST_MODE_HANDGRIP
};

// Stato iniziale (in setup): tempi a zero, nessuna manovra attiva
void initFSM();

// Da chiamare a ogni loop
void updateFSM();

// Cambia la manovra attiva (con lampeggio di conferma, tranne che per IDLE)
void setTestMode(SystemTestMode newMode);

#endif
