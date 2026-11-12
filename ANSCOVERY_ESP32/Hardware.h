#ifndef HARDWARE_H
#define HARDWARE_H

#include "Config.h"

enum MarkerEvent {
    EVENT_NONE,
    EVENT_SINGLE_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS      // generato appena la pressione supera BTN_LONG_PRESS_MS
};

// Valori dei sensori aggiornati da sampleSensors() a 50 Hz
struct SensorData {
    float tiltAngle;        // gradi, filtrato
    float tiltRate;         // gradi/s (positivo = salita), su finestra di 1 s
    bool  tiltOk;           // false se l'MPU-6050 non risponde
    float valsalvaMmHg;     // pressione SP1
    float handgripMmHg;     // pressione assoluta del manicotto
    float handgripDelta;    // pressione sopra il precarico
};

void initHardware();

// Campiona i sensori (non bloccante, internamente limitato a SENSOR_SAMPLE_MS)
void sampleSensors();
const SensorData& sensors();

// Il precarico del manicotto viene inseguito solo quando abilitato (in IDLE)
void setHandgripBaselineTracking(bool enabled);

bool isBtnAzzeraT1Pressed();
MarkerEvent checkMarkerBtnEvent();

#endif
