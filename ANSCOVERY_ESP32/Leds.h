// =====================================================================
// Leds.h - Barra LED WS2812B (feedback visivo per paziente e operatore)
// =====================================================================

#ifndef ANSCOVERY_LEDS_H
#define ANSCOVERY_LEDS_H

#include <Arduino.h>

void initLeds();
void clearLeds();

// Lampeggio del colore indicato (bloccante, LED_BLINK_COUNT lampeggi)
void blinkLedsConfirm(uint8_t r, uint8_t g, uint8_t b);

// Barra in IDLE: spenta
void updateIdleLeds();

// Ritmo del Deep Breathing; con active = false la barra si spegne
void updateDeepBreathingLeds(bool active);

// Barra della Valsalva rispetto al target di VALSALVA_TARGET_MMHG
void updateValsalvaLeds(float pressure);

// Barra dell'Hand Grip: fondo scala = media dei picchi, target = 1/3 della media
void updateHandgripLeds(float force, float mediaPeaks, float targetForce);

#endif
