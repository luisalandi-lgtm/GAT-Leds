#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

// Gestione barra LED WS2812B (6 barre da 8 LED)
void initLeds();
void clearLeds();

// Lampeggio di conferma all'avvio di un test (non bloccante)
void startConfirmBlink(uint8_t r, uint8_t g, uint8_t b);
// Disegna il lampeggio se attivo; ritorna true finche' e' in corso
bool updateConfirmBlink();

// level = 0..1 (stesso valore inviato su OUT2)
void updateDeepBreathingLeds(float level);
void updateValsalvaLeds(float pressure);
void updateHandgripLeds(float delta, float mediaPeaks, float target);

#endif
