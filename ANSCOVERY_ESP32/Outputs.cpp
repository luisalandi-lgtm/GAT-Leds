// =====================================================================
// Outputs.cpp - Uscite DAC OUT1 (marker) e OUT2 (segnale continuo)
// =====================================================================

#include "Outputs.h"

// Marker in corso su OUT1
static unsigned long markerStartTime = 0;
static unsigned long markerDuration = 0;
static bool markerActive = false;

// Ultimi valori scritti sui DAC (0..255), per la stampa su seriale
static uint8_t out1Value = 0;
static uint8_t out2Value = 0;

static void writeOutput(uint8_t pin, uint8_t value) {
    dacWrite(pin, value);
    if (pin == PIN_DAC_OUT1) out1Value = value;
    if (pin == PIN_DAC_OUT2) out2Value = value;
}

void triggerMarker(float voltage, unsigned long durationMs) {
    if (markerActive) return;

    // Tensione -> valore DAC (DAC_FULL_SCALE_V = 255); oltre il fondo scala viene limitata
    int dacValue = (int)((voltage / DAC_FULL_SCALE_V) * 255.0f);
    if (dacValue > DAC_MAX_VALUE) dacValue = DAC_MAX_VALUE;

    writeOutput(PIN_DAC_OUT1, dacValue);
    markerStartTime = millis();
    markerDuration = durationMs;
    markerActive = true;
}

void updateMarker() {
    if (markerActive && (millis() - markerStartTime >= markerDuration)) {
        writeOutput(PIN_DAC_OUT1, 0);
        markerActive = false;
    }
}

void writeOut2Normalized(float normalized) {
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;
    writeOutput(PIN_DAC_OUT2, (uint8_t)(normalized * 255.0f));
}

// Tensione stampata = valore DAC * 3,3 V / 255 (tensione sul pin)
void printOutputs() {
    static uint8_t lastOut1 = 0;
    static unsigned long lastPrint = 0;
    if (out1Value != lastOut1 || millis() - lastPrint >= OUTPUTS_PRINT_INTERVAL_MS) {
        lastOut1 = out1Value;
        lastPrint = millis();
        Serial.print("[USCITE] OUT1: ");
        Serial.print(out1Value * DAC_FULL_SCALE_V / 255.0f, 2);
        Serial.print(" V (DAC ");
        Serial.print(out1Value);
        Serial.print(") | OUT2: ");
        Serial.print(out2Value * DAC_FULL_SCALE_V / 255.0f, 2);
        Serial.print(" V (DAC ");
        Serial.print(out2Value);
        Serial.println(")");
    }
}
