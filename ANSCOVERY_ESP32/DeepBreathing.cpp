// =====================================================================
// DeepBreathing.cpp - Deep Breathing (respirazione guidata)
// 1. Doppio click in IDLE: la barra LED guida il ritmo (5 s su, 5 s giu').
//    OUT2 = sinusoide del ritmo, da 0 (inizio inspirazione) al massimo (fine inspirazione).
// 2. Click singolo: inizio registrazione, marker 4 V / 1 s, T2 riparte.
// 3. Dopo 10 respiri (100 s): marker 4 V / 2 s, ritorno in IDLE.
// Doppio click durante la manovra: stop senza marker.
// I click entro DEEP_BREATHING_LOCKOUT_MS dall'avvio vengono ignorati.
// =====================================================================

#include "Maneuvers.h"
#include "FSM.h"
#include "Outputs.h"
#include "Timers.h"
#include "Leds.h"

static bool ledsActive = false;      // barra LED del ritmo attiva
static bool recording = false;       // registrazione dei 10 respiri in corso
static uint32_t startTime = 0;       // istante del doppio click di avvio

void resetDeepBreathing() {
    ledsActive = false;
    recording = false;
}

bool isDeepBreathingLedsActive() {
    return ledsActive;
}

void startDeepBreathing() {
    startTime = millis();
    setTestMode(TEST_MODE_DEEP_BREATHING);
    ledsActive = true;
    recording = false;
}

static bool lockoutElapsed() {
    return millis() - startTime >= DEEP_BREATHING_LOCKOUT_MS;
}

void deepBreathingOnClick() {
    if (lockoutElapsed()) {
        if (ledsActive && !recording) {
            recording = true;
            restartTempo2();

            Serial.println(">> DEEP BREATHING: Click -> Inizio Registrazione (10 atti / 1m40s) - Marker 4.0V (1s)");
            triggerMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_START);
        }
    }
}

void deepBreathingOnDoubleClick() {
    if (lockoutElapsed()) {
        ledsActive = false;
        recording = false;
        stopTempo2();
        clearLeds();
        Serial.println(">> DEEP BREATHING: Doppio Click -> Stop Manuale");
        setTestMode(TEST_MODE_IDLE);
    }
}

void updateDeepBreathing() {
    writeOut2Normalized(getBreathingLevel());

    if (recording) {
        uint32_t elapsedTime = tempo2ElapsedMs();
        if (elapsedTime >= DEEP_BREATHING_TOTAL_MS) {
            recording = false;
            ledsActive = false;
            stopTempo2();

            Serial.println(">> DEEP BREATHING: 10 Atti Completati -> Marker Fine 4.0V (2s) e Stop LED");
            triggerMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_END);
            setTestMode(TEST_MODE_IDLE);
        }
    }
}
