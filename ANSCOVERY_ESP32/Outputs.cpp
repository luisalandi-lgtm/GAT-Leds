#include "Outputs.h"

struct MarkerPulse {
    float volts;
    uint32_t durationMs;
};

static const uint8_t MARKER_QUEUE_LEN = 8;
static MarkerPulse markerQueue[MARKER_QUEUE_LEN];
static uint8_t queueHead = 0;
static uint8_t queueCount = 0;

enum MarkerPhase { MARKER_IDLE, MARKER_PULSE, MARKER_GAP };
static MarkerPhase markerPhase = MARKER_IDLE;
static uint32_t phaseStart = 0;
static uint32_t phaseDuration = 0;

static float out1Volts = 0.0f;
static float out2Volts = 0.0f;
static int lastDac1 = -1;
static int lastDac2 = -1;

static uint8_t voltsToDac(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > OUT_FULL_SCALE_V) v = OUT_FULL_SCALE_V;
    return (uint8_t)(v / OUT_FULL_SCALE_V * 255.0f + 0.5f);
}

static void writeDacs() {
    int d1 = voltsToDac(out1Volts);
    int d2 = voltsToDac(out2Volts);
    if (d1 != lastDac1) { dacWrite(PIN_DAC_OUT1, d1); lastDac1 = d1; }
    if (d2 != lastDac2) { dacWrite(PIN_DAC_OUT2, d2); lastDac2 = d2; }
}

void initOutputs() {
    out1Volts = 0.0f;
    out2Volts = 0.0f;
    writeDacs();
}

void queueMarker(float volts, uint32_t durationMs) {
    if (queueCount >= MARKER_QUEUE_LEN) {
        LOGF("!! Coda marker piena: marker %.2f V scartato\n", volts);
        return;
    }
    uint8_t idx = (queueHead + queueCount) % MARKER_QUEUE_LEN;
    markerQueue[idx].volts = volts;
    markerQueue[idx].durationMs = durationMs;
    queueCount++;
}

void setOut2(float volts) {
    if (volts < 0.0f) volts = 0.0f;
    if (volts > OUT_FULL_SCALE_V) volts = OUT_FULL_SCALE_V;
    out2Volts = volts;
}

static void startNextPulse(uint32_t now) {
    MarkerPulse p = markerQueue[queueHead];
    queueHead = (queueHead + 1) % MARKER_QUEUE_LEN;
    queueCount--;
    out1Volts = p.volts;
    markerPhase = MARKER_PULSE;
    phaseStart = now;
    phaseDuration = p.durationMs;
    LOGF("[OUT1] %.2f V per %lu ms\n", p.volts, (unsigned long)p.durationMs);
}

void updateOutputs() {
    uint32_t now = millis();

    switch (markerPhase) {
        case MARKER_IDLE:
            if (queueCount > 0) startNextPulse(now);
            break;

        case MARKER_PULSE:
            if (now - phaseStart >= phaseDuration) {
                out1Volts = 0.0f;
                LOGF("[OUT1] 0.00 V\n");
                markerPhase = MARKER_GAP;
                phaseStart = now;
                phaseDuration = MARKER_GAP_MS;
            }
            break;

        case MARKER_GAP:
            if (now - phaseStart >= phaseDuration) {
                markerPhase = MARKER_IDLE;
                if (queueCount > 0) startNextPulse(now);
            }
            break;
    }

    writeDacs();

#if SERIAL_PLOTTER_MODE
    static uint32_t lastPlot = 0;
    if (now - lastPlot >= PLOTTER_INTERVAL_MS) {
        lastPlot = now;
        Serial.printf("OUT1:%.2f,OUT2:%.2f\n", out1Volts, out2Volts);
    }
#endif
}

float getOut1Volts() { return out1Volts; }
float getOut2Volts() { return out2Volts; }
