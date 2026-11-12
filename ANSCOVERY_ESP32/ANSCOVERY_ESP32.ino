#include "Config.h"
#include "Hardware.h"
#include "Outputs.h"
#include "FSM.h"
#include "Display.h"
#include "Leds.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    LOGF("\nANSCOVERY ESP32 - Avvio sistema...\n");

    initOutputs();
    initHardware();
    initLeds();
    initDisplayHardware();
    initFSM();
}

void loop() {
    updateFSM();
    updateDisplays(getTempo1Seconds(), getTempo2Seconds());
}
