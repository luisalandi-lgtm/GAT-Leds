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
#endif