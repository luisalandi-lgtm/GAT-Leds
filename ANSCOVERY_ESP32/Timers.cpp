// =====================================================================
// Timers.cpp - Tempo 1 e Tempo 2 (in millisecondi)
// StartMs = istante di partenza, Running = sta contando,
// FrozenMs = valore mostrato quando il tempo e' fermo.
// =====================================================================

#include "Timers.h"

static uint32_t tempo1StartMs = 0;
static bool tempo1Running = false;
static uint32_t tempo1FrozenMs = 0;

static uint32_t tempo2StartMs = 0;
static bool tempo2Running = false;
static uint32_t tempo2FrozenMs = 0;

void initTimers() {
    tempo1StartMs = 0;
    tempo1Running = false;
    tempo1FrozenMs = 0;
    tempo2StartMs = 0;
    tempo2Running = false;
    tempo2FrozenMs = 0;
}

void toggleTempo1() {
    if (!tempo1Running) {
        tempo1StartMs = millis();
        tempo1Running = true;
        tempo1FrozenMs = 0;
        Serial.println(">> T1 OPERATORE: Avviato");
    } else {
        tempo1FrozenMs = millis() - tempo1StartMs;
        tempo1Running = false;
        Serial.print(">> T1 OPERATORE: Congelato a ");
        Serial.print(tempo1FrozenMs / 1000.0f, 1);
        Serial.println(" s");
    }
}

void restartTempo2() {
    tempo2StartMs = millis();
    tempo2Running = true;
}

void restartTempo2ClearFrozen() {
    tempo2StartMs = millis();
    tempo2Running = true;
    tempo2FrozenMs = 0;
}

void stopTempo2() {
    tempo2Running = false;
    tempo2StartMs = 0;
}

void freezeTempo2Excluding(uint32_t excludedMs) {
    tempo2FrozenMs = millis() - tempo2StartMs - excludedMs;
    tempo2Running = false;
    tempo2StartMs = 0;
}

uint32_t tempo2ElapsedMs() {
    return millis() - tempo2StartMs;
}

float getTempo1Seconds() {
    if (tempo1Running) {
        return (millis() - tempo1StartMs) / 1000.0f;
    } else {
        return tempo1FrozenMs / 1000.0f;
    }
}

float getTempo2Seconds() {
    if (tempo2Running) {
        return (millis() - tempo2StartMs) / 1000.0f;
    } else {
        return tempo2FrozenMs / 1000.0f;
    }
}
