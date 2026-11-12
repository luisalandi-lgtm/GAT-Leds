#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <string.h>
#include "Leds.h"
#include "Config.h"

static Adafruit_NeoPixel strip(NUM_LEDS, PIN_LED_BAR, NEO_GRB + NEO_KHZ800);

// Copia dell'ultimo frame inviato: show() solo se qualcosa e' cambiato
static uint8_t lastFrame[NUM_LEDS * 3];

static const uint8_t  BLINK_COUNT  = 2;
static const uint16_t BLINK_ON_MS  = 150;
static const uint16_t BLINK_OFF_MS = 150;
static bool blinkActive = false;
static uint32_t blinkStart = 0;
static uint32_t blinkColor = 0;

static void render() {
    uint8_t *px = strip.getPixels();
    if (memcmp(px, lastFrame, sizeof(lastFrame)) != 0) {
        strip.show();
        memcpy(lastFrame, px, sizeof(lastFrame));
    }
}

void initLeds() {
    strip.begin();
    strip.setBrightness(LED_BRIGHTNESS);
    strip.clear();
    strip.show();
    memset(lastFrame, 0, sizeof(lastFrame));
}

void clearLeds() {
    strip.clear();
    render();
}

void startConfirmBlink(uint8_t r, uint8_t g, uint8_t b) {
    blinkActive = true;
    blinkStart = millis();
    blinkColor = strip.Color(r, g, b);
}

bool updateConfirmBlink() {
    if (!blinkActive) return false;

    uint32_t elapsed = millis() - blinkStart;
    uint32_t period = BLINK_ON_MS + BLINK_OFF_MS;
    if (elapsed >= (uint32_t)BLINK_COUNT * period) {
        blinkActive = false;
        clearLeds();
        return false;
    }
    if ((elapsed % period) < BLINK_ON_MS) {
        strip.fill(blinkColor);
    } else {
        strip.clear();
    }
    render();
    return true;
}

static void drawLevel(int lit, uint32_t color) {
    if (lit < 0) lit = 0;
    if (lit > NUM_LEDS) lit = NUM_LEDS;
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, i < lit ? color : 0);
    }
}

void updateDeepBreathingLeds(float level) {
    drawLevel((int)(level * NUM_LEDS + 0.5f), strip.Color(0, 100, 255));
    render();
}

// Barra con indicatore del target: rosso sotto target, verde sopra, target giallo
static void drawTargetBar(float value, float fullScale, float target) {
    int lit = (int)((value / fullScale) * NUM_LEDS);
    int targetIdx = (int)((target / fullScale) * NUM_LEDS);
    if (targetIdx >= NUM_LEDS) targetIdx = NUM_LEDS - 1;
    bool reached = (value >= target);

    for (int i = 0; i < NUM_LEDS; i++) {
        if (i == targetIdx) {
            strip.setPixelColor(i, strip.Color(255, 255, 0));
        } else if (i < lit) {
            strip.setPixelColor(i, reached ? strip.Color(0, 255, 0) : strip.Color(255, 0, 0));
        } else {
            strip.setPixelColor(i, 0);
        }
    }
}

void updateValsalvaLeds(float pressure) {
    if (pressure < 5.0f) {
        clearLeds();
        return;
    }
    drawTargetBar(pressure, VALSALVA_LED_SCALE, VALSALVA_TARGET);
    render();
}

void updateHandgripLeds(float delta, float mediaPeaks, float target) {
    if (mediaPeaks <= 0.0f || delta < 1.0f) {
        clearLeds();
        return;
    }
    drawTargetBar(delta, mediaPeaks, target);
    render();
}
