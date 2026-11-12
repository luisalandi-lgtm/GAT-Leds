#ifndef FSM_H
#define FSM_H

#include "Config.h"

// Definizione delle modalità di test attive
enum SystemTestMode {
    TEST_MODE_IDLE,
    TEST_MODE_TILT,
    TEST_MODE_DEEP_BREATHING,
    TEST_MODE_VALSALVA,
    TEST_MODE_HANDGRIP
};

extern SystemTestMode currentTestMode;

// Funzioni per l'avvio dinamico dei test
void setTestMode(SystemTestMode newMode);

bool isDeepBreathingActive();
void initFSM();
void updateFSM();
void updateValsalvaFSM(); 
void updateHandgripFSM();
void resetHandgripCalib(); // Per resettare le 3 strette se necessario

extern uint32_t tempo2StartMs;
extern bool tempo2Running;
extern uint32_t tempo2FrozenMs; // Tempo congelato a fine manovra

extern uint32_t tempo1StartMs;
extern bool tempo1Running;
extern uint32_t tempo1FrozenMs;

float getTempo1Seconds();
float getTempo2Seconds();

#endif