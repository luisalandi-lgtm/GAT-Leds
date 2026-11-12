#ifndef FSM_H
#define FSM_H

#include "Config.h"

enum SystemTestMode {
    TEST_MODE_IDLE,
    TEST_MODE_TILT,
    TEST_MODE_DEEP_BREATHING,
    TEST_MODE_VALSALVA,
    TEST_MODE_HANDGRIP,
    TEST_MODE_COLD_FACE
};

void initFSM();
void updateFSM();

SystemTestMode getTestMode();

uint32_t getTempo1Seconds();
uint32_t getTempo2Seconds();

#endif
