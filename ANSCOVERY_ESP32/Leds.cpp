// =====================================================================
// Leds.cpp - Barra LED WS2812B
// Lampeggio di conferma all'avvio di una manovra, ritmo del Deep
// Breathing, barre con target per Valsalva e Hand Grip.
// =====================================================================

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Leds.h"
#include "Config.h"

static Adafruit_NeoPixel strip(NUM_LEDS, PIN_LED_BAR, NEO_GRB + NEO_KHZ800);

// Inizio del ritmo del Deep Breathing (0 = ritmo non avviato)
static uint32_t breathStartTime = 0;

void initLeds() {
    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void clearLeds() {
    strip.clear();
    strip.show();
}

void blinkLedsConfirm(uint8_t r, uint8_t g, uint8_t b) {
    for (int k = 0; k < LED_BLINK_COUNT; k++) {
        strip.fill(strip.Color(r, g, b));
        strip.show();
        delay(LED_BLINK_MS);
        strip.clear();
        strip.show();
        delay(LED_BLINK_MS);
    }
}

void updateIdleLeds() {
    clearLeds();
}

// In BREATH_IN_TIME_MS la barra si riempie, in BREATH_OUT_TIME_MS si svuota.
// Il ritmo parte alla prima chiamata con active = true.
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
            strip.setPixelColor(i, strip.Color(0, 100, 255)); // blu
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();
}

float getBreathingLevel() {
    if (breathStartTime == 0) return 0.0f;

    uint32_t t = (millis() - breathStartTime) % BREATH_CYCLE_MS;
    if (t < BREATH_IN_TIME_MS) {
        return 0.5f * (1.0f - cosf((float)M_PI * t / BREATH_IN_TIME_MS));
    }
    return 0.5f * (1.0f + cosf((float)M_PI * (t - BREATH_IN_TIME_MS) / BREATH_OUT_TIME_MS));
}

// Barra proporzionale a value su fullScale, con un LED giallo fisso sul target:
// rossa sotto il target, verde dal target in su.
static void drawTargetBar(float value, float fullScale, float target) {
    int totalLeds = strip.numPixels();

    int ledsToLight = (int)((value / fullScale) * totalLeds);
    if (ledsToLight > totalLeds) ledsToLight = totalLeds;

    int targetLedIndex = (int)((target / fullScale) * totalLeds);
    bool targetReached = (value >= target);

    for (int i = 0; i < totalLeds; i++) {
        if (i == targetLedIndex) {
            strip.setPixelColor(i, strip.Color(255, 255, 0));     // giallo: target
        } else if (i < ledsToLight) {
            if (targetReached) {
                strip.setPixelColor(i, strip.Color(0, 255, 0));   // verde
            } else {
                strip.setPixelColor(i, strip.Color(255, 0, 0));   // rosso
            }
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));         // spento
        }
    }
    strip.show();
}

void updateValsalvaLeds(float pressure) {
    if (pressure < VALSALVA_LED_MIN_MMHG) {
        clearLeds();
        return;
    }
    drawTargetBar(pressure, VALSALVA_LED_FULL_SCALE_MMHG, VALSALVA_TARGET_MMHG);
}

void updateHandgripLeds(float force, float mediaPeaks, float targetForce) {
    if (mediaPeaks <= 0.0f || force < HANDGRIP_LED_MIN_KG) {
        clearLeds();
        return;
    }
    drawTargetBar(force, mediaPeaks, targetForce);
}
