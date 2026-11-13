// =====================================================================
// ColdFace.cpp - Cold Face Test
// 1. Pressione lunga del MARKER in IDLE: lampeggio ciano, poi
//    marker 5 V / 1 s e T2 riparte.
// 2. Click singolo: fine del test, marker 5 V / 2 s, T2 fermo sulla
//    durata del test, ritorno in IDLE.
// OUT2 = 0 V per tutto il test.
// =====================================================================

#include "Maneuvers.h"
#include "FSM.h"
#include "Outputs.h"
#include "Timers.h"

void startColdFace() {
    setTestMode(TEST_MODE_COLD_FACE);
    restartTempo2();
    Serial.println(">> COLD FACE: Inizio -> Marker 5.0V (1s) & Azzera T2");
    triggerMarker(VOLT_MARKER_COLD, DURATION_MARKER_START);
}

void coldFaceOnClick() {
    freezeTempo2Excluding(0);

    Serial.print(">> COLD FACE: Click -> FINE TEST! T2 Finale: ");
    Serial.print(getTempo2Seconds(), 1);
    Serial.println(" s | Marker 5.0V (2s)");

    triggerMarker(VOLT_MARKER_COLD, DURATION_MARKER_END);
    setTestMode(TEST_MODE_IDLE);
}

void updateColdFace() {
    writeOut2Normalized(0.0f);
}
