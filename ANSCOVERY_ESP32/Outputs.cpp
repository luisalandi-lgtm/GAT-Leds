// =====================================================================
// Outputs.cpp - Uscite DAC OUT1 (marker) e OUT2 (segnale continuo)
// =====================================================================

#include "Outputs.h"

// Marker in corso su OUT1
static unsigned long markerStartTime = 0;
static unsigned long markerDuration = 0;
static bool markerActive = false;

// Fine dell'ultimo marker (per la pausa a 0 V prima del successivo)
static unsigned long markerEndTime = 0;
static bool markerEverEnded = false;

// Coda circolare dei marker in attesa
struct MarkerRequest {
    float voltage;
    unsigned long durationMs;
};
static MarkerRequest markerQueue[MARKER_QUEUE_SIZE];
static uint8_t queueHead = 0;
static uint8_t queueCount = 0;

// Ultimi valori scritti sui DAC (0..255), per la stampa su seriale
static uint8_t out1Value = 0;
static uint8_t out2Value = 0;

static void writeOutput(uint8_t pin, uint8_t value) {
    dacWrite(pin, value);
    if (pin == PIN_DAC_OUT1) out1Value = value;
    if (pin == PIN_DAC_OUT2) out2Value = value;
}

void initOutputs() {
    writeOutput(PIN_DAC_OUT1, 0);
    writeOutput(PIN_DAC_OUT2, 0);
}

static void startMarker(float voltage, unsigned long durationMs) {
    // Tensione -> valore DAC (DAC_FULL_SCALE_V = 255); oltre il fondo scala viene limitata
    int dacValue = (int)((voltage / DAC_FULL_SCALE_V) * 255.0f);
    if (dacValue > DAC_MAX_VALUE) dacValue = DAC_MAX_VALUE;

    writeOutput(PIN_DAC_OUT1, dacValue);
    markerStartTime = millis();
    markerDuration = durationMs;
    markerActive = true;
}

// true se e' trascorsa la pausa a 0 V dopo l'ultimo marker
static bool markerGapElapsed() {
    return !markerEverEnded || (millis() - markerEndTime >= MARKER_GAP_MS);
}

void triggerMarker(float voltage, unsigned long durationMs) {
    // OUT1 libero: il marker parte subito
    if (!markerActive && queueCount == 0 && markerGapElapsed()) {
        startMarker(voltage, durationMs);
        return;
    }

    // Altrimenti va in coda
    if (queueCount >= MARKER_QUEUE_SIZE) {
        Serial.println(">> MARKER: coda piena, marker scartato");
        return;
    }
    uint8_t tail = (queueHead + queueCount) % MARKER_QUEUE_SIZE;
    markerQueue[tail].voltage = voltage;
    markerQueue[tail].durationMs = durationMs;
    queueCount++;
}

void updateMarker() {
    // Fine del marker in corso
    if (markerActive && (millis() - markerStartTime >= markerDuration)) {
        writeOutput(PIN_DAC_OUT1, 0);
        markerActive = false;
        markerEndTime = millis();
        markerEverEnded = true;
    }

    // Prossimo marker in coda, dopo la pausa a 0 V
    if (!markerActive && queueCount > 0 && markerGapElapsed()) {
        MarkerRequest next = markerQueue[queueHead];
        queueHead = (queueHead + 1) % MARKER_QUEUE_SIZE;
        queueCount--;
        startMarker(next.voltage, next.durationMs);
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
