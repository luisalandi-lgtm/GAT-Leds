#ifndef HARDWARE_H
#define HARDWARE_H

#include "Config.h"
#include <Wire.h>

enum MarkerEvent {
    EVENT_NONE,
    EVENT_SINGLE_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS
};

void initHardware();
float readTiltAngle();
float readValsalvaPressure();
float readHandGripPressure();
bool isBtnAzzeraT1Pressed();

MarkerEvent checkMarkerBtnEvent();
void triggerMarker(float voltage, unsigned long durationMs);
void updateMarker();

// Scrive il DAC e memorizza il valore per la stampa su seriale
void writeOutput(uint8_t pin, uint8_t value);
void printOutputs();
#endif