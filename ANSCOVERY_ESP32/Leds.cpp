// =====================================================================
// Leds.cpp - Barra LED WS2812B
// Lampeggio di conferma all'avvio di una manovra, ritmo del Deep
// Breathing, barre con target per Valsalva e Hand Grip.
// =====================================================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Leds.h"
#include "Config.h"

// Barra LED: NUM_LEDS pixel sul pin PIN_LED_BAR, colori in ordine GRB
static Adafruit_NeoPixel strip(NUM_LEDS, PIN_LED_BAR, NEO_GRB + NEO_KHZ800);
// Inizio del ritmo del Deep Breathing (0 = ritmo non avviato)
static uint32_t breathStartTime = 0;

void initLedsHardware() {
    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void initLeds() {
    initLedsHardware();
}

void clearLeds() {
    strip.clear();
    strip.show();
}

// Due lampeggi del colore indicato (120 ms acceso, 120 ms spento). Bloccante: circa 480 ms.
void blinkLedsConfirm(uint8_t r, uint8_t g, uint8_t b) {
    for (int k = 0; k < 2; k++) {
        strip.fill(strip.Color(r, g, b));
        strip.show();
        delay(120);
        strip.clear();
        strip.show();
        delay(120);
    }
}

// Ritmo respiratorio: in BREATH_IN_TIME_MS la barra si riempie, in BREATH_OUT_TIME_MS
// si svuota. Con active = false la barra si spegne e il ritmo riparte da capo.
void updateDeepBreathingLeds(bool active) {
    if (!active) {
        if (breathStartTime != 0) {
            breathStartTime = 0;
            clearLeds();
        }
        return;
    }

    if (breathStartTime == 0) {
        breathStartTime = millis();
    }

    uint32_t elapsedTime = (millis() - breathStartTime) % BREATH_CYCLE_MS;
    int ledsToLight = 0;

    if (elapsedTime < BREATH_IN_TIME_MS) {
        ledsToLight = map(elapsedTime, 0, BREATH_IN_TIME_MS, 0, NUM_LEDS);
    } else {
        uint32_t breathOutTime = elapsedTime - BREATH_IN_TIME_MS;
        ledsToLight = map(breathOutTime, 0, BREATH_OUT_TIME_MS, NUM_LEDS, 0);
    }

    if (ledsToLight > NUM_LEDS) ledsToLight = NUM_LEDS;
    if (ledsToLight < 0) ledsToLight = 0;

    for (int i = 0; i < NUM_LEDS; i++) {
        if (i < ledsToLight) {
            strip.setPixelColor(i, strip.Color(0, 100, 255)); // BLU Deep Breathing
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));     // SPENTO FORZATO
        }
    }
    strip.show();
}

// Barra della Valsalva: fondo scala VALSALVA_MAX_SCALE, LED giallo sul target,
// rosso sotto il target, verde dal target in su. Spenta sotto 5 mmHg.
void updateValsalvaLeds(float pressure) {
    // SOGLIA MINIMA: Se la pressione è sotto 5.0 mmHg, spegni i LED
    if (pressure < 5.0f) { 
        clearLeds();
        return;
    }

    int totalLeds = strip.numPixels();

    int ledsToLight = (int)((pressure / VALSALVA_MAX_SCALE) * totalLeds);
    if (ledsToLight > totalLeds) ledsToLight = totalLeds;

    int targetLedIndex = (int)((TARGET_VALSALVA / VALSALVA_MAX_SCALE) * totalLeds);

    // Controlla se il target è stato raggiunto o superato
    bool targetReached = (pressure >= TARGET_VALSALVA);

    for (int i = 0; i < totalLeds; i++) {
        if (i == targetLedIndex) {
            // Il LED di riferimento del target rimane sempre GIALLO
            strip.setPixelColor(i, strip.Color(255, 255, 0)); 
        } else if (i < ledsToLight) {
            if (targetReached) {
                strip.setPixelColor(i, strip.Color(0, 255, 0)); // VERDE se target raggiunto
            } else {
                strip.setPixelColor(i, strip.Color(255, 0, 0)); // ROSSO sotto il target
            }
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));     // SPENTO
        }
    }
    strip.show();
}

// Barra dell'Hand Grip: fondo scala = media dei picchi, LED giallo sul target,
// rosso sotto il target, verde dal target in su. Spenta sotto 1 kg.
void updateHandgripLeds(float currentForce, float maxMedia, float targetForce) {
    // SOGLIA MINIMA: Spegni i LED se la forza è trascurabile o se la media non è valida
    if (maxMedia <= 0.0f || currentForce < 1.0f) { 
        clearLeds();
        return;
    }

    int totalLeds = strip.numPixels();

    int ledsToLight = (int)((currentForce / maxMedia) * totalLeds);
    if (ledsToLight > totalLeds) ledsToLight = totalLeds;

    int targetLedIndex = (int)((targetForce / maxMedia) * totalLeds);

    // Controlla se il target è stato raggiunto o superato
    bool targetReached = (currentForce >= targetForce);

    for (int i = 0; i < totalLeds; i++) {
        if (i == targetLedIndex) {
            // Il LED di riferimento del target rimane sempre GIALLO
            strip.setPixelColor(i, strip.Color(255, 255, 0)); 
        } else if (i < ledsToLight) {
            if (targetReached) {
                strip.setPixelColor(i, strip.Color(0, 255, 0)); // VERDE se target raggiunto
            } else {
                strip.setPixelColor(i, strip.Color(255, 0, 0)); // ROSSO sotto il target
            }
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));     // SPENTO
        }
    }
    strip.show();
}

void updateIdleLeds() {
    // In IDLE la barra rimane totalmente spenta
    clearLeds();
}