// Hardware.h - Sensori, pulsanti e uscite DAC (OUT1 marker, OUT2 continuo)

#ifndef HARDWARE_H
#define HARDWARE_H

#include "Config.h"
#include <Wire.h>

// Eventi riconosciuti sul pulsante MARKER
enum MarkerEvent {
    EVENT_NONE,
    EVENT_SINGLE_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS
};

void initHardware();
// Letture dei sensori: angolo in gradi, Valsalva in mmHg, Hand Grip in kg
float readTiltAngle();
float readValsalvaPressure();
float readHandGripPressure();
bool isBtnAzzeraT1Pressed();

// Da chiamare a ogni loop: restituisce un evento solo nel ciclo in cui viene riconosciuto
MarkerEvent checkMarkerBtnEvent();
// Marker su OUT1: avvio (tensione e durata) e spegnimento a tempo scaduto
void triggerMarker(float voltage, unsigned long durationMs);
void updateMarker();

// Scrive il DAC e memorizza il valore per la stampa su seriale
void writeOutput(uint8_t pin, uint8_t value);
void printOutputs();
#endif