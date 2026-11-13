// =====================================================================
// TiltTest.cpp - Tilt Test
// OUT2 = angolo del lettino in continuo (0..TILT_OUT2_FULL_SCALE_DEG).
// Le fasi vengono valutate una volta al secondo, confrontando l'angolo
// con quello del secondo precedente:
//   TILT_IDLE     -> TILT_RISING    angolo >= 30 e in salita: marker 1 V / 1 s
//   TILT_RISING   -> TILT_VERTICAL  angolo >= 55 fermo per 2 s: marker 1 V / 2 s, T2 riparte
//   TILT_VERTICAL -> TILT_FALLING   angolo < 55: marker 1 V / 1 s, T2 si ferma
//   TILT_FALLING  -> IDLE           angolo < 5 fermo per 2 s: marker 1 V / 2 s
// =====================================================================

#include "Maneuvers.h"
#include "FSM.h"
#include "Sensors.h"
#include "Outputs.h"
#include "Timers.h"

enum TiltPhase {
    TILT_IDLE,       // orizzontale, in attesa della salita
    TILT_RISING,     // salita in corso
    TILT_VERTICAL,   // verticale stabile (T2 in conteggio)
    TILT_FALLING     // discesa in corso
};

static TiltPhase currentTiltPhase = TILT_IDLE;
static uint32_t tiltStabilityTimer = 0;  // istante da cui l'angolo e' stabile
static float angleOneSecondAgo = 0.0f;   // angolo al controllo precedente
static uint32_t lastTiltCheckTime = 0;

void resetTiltPhase() {
    currentTiltPhase = TILT_IDLE;
}

void updateTiltTest() {
    float angle = readTiltAngle();
    writeOut2Normalized(angle / TILT_OUT2_FULL_SCALE_DEG);

    // Da qui in poi solo una volta al secondo
    if (millis() - lastTiltCheckTime < TILT_CHECK_INTERVAL_MS) {
        return;
    }
    lastTiltCheckTime = millis();

    // Variazione nell'ultimo secondo (gradi/s): > 0 salita, < 0 discesa
    float deltaAnglePerSec = angle - angleOneSecondAgo;

    switch (currentTiltPhase) {
        case TILT_IDLE:
            if (angle >= TILT_START_DEG && deltaAnglePerSec > TILT_RATE_DEG_S) {
                currentTiltPhase = TILT_RISING;
                tiltStabilityTimer = millis();
                Serial.println(">> TILT: Inizio Salita (>30°) -> Marker 1.0V (1s)");
                triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
            }
            break;

        case TILT_RISING:
            if (angle >= TILT_VERTICAL_DEG && abs(deltaAnglePerSec) < TILT_RATE_DEG_S) {
                if (millis() - tiltStabilityTimer >= TILT_STABLE_TIME_MS) {
                    currentTiltPhase = TILT_VERTICAL;
                    restartTempo2();
                    Serial.println(">> TILT: Posizione Verticale Raggiunta (>=55°) -> Marker 1.0V (2s) & AVVIO T2");
                    triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                }
            } else {
                tiltStabilityTimer = millis();
            }
            break;

        case TILT_VERTICAL:
            if (angle < TILT_VERTICAL_DEG) {
                currentTiltPhase = TILT_FALLING;
                tiltStabilityTimer = millis();
                stopTempo2();
                Serial.println(">> TILT: Inizio Discesa (<55°) -> Marker 1.0V (1s) & STOP T2");
                triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
            }
            break;

        case TILT_FALLING:
            if (angle < TILT_HORIZONTAL_DEG && abs(deltaAnglePerSec) < TILT_RATE_DEG_S) {
                if (millis() - tiltStabilityTimer >= TILT_STABLE_TIME_MS) {
                    currentTiltPhase = TILT_IDLE;
                    Serial.println(">> TILT: Fine Manovra (Orizzontale <5°) -> Marker 1.0V (2s)");
                    triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                    setTestMode(TEST_MODE_IDLE);
                }
            } else {
                tiltStabilityTimer = millis();
            }
            break;
    }

    angleOneSecondAgo = angle;
}
