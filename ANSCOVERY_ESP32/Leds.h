#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

// Funzioni di gestione LED WS2812
void initLeds();
void clearLeds();
void blinkLedsConfirm(uint8_t r, uint8_t g, uint8_t b);
void updateIdleLeds();
void updateDeepBreathingLeds(bool active);
void updateValsalvaLeds(float pressure);
void updateHandgripLeds(float force, float mediaPeaks, float targetForce);

#endif