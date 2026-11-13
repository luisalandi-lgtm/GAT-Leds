// =====================================================================
// Valsalva.cpp - Manovra di Valsalva
// OUT2 = pressione SP1 in continuo (0..VALSALVA_MAX_MMHG).
//   VALSALVA_IDLE   -> VALSALVA_ACTIVE  pressione >= 25 mmHg: marker 2 V / 1 s, T2 riparte
//   VALSALVA_ACTIVE -> IDLE             pressione < 10 mmHg per 3 s: marker 2 V / 2 s,
//                                       T2 fermo sulla durata del soffio (3 s esclusi)
// =====================================================================

#include "Maneuvers.h"
#include "FSM.h"
#include "Sensors.h"
#include "Outputs.h"
#include "Timers.h"

enum ValsalvaPhase {
    VALSALVA_IDLE,    // in attesa del superamento di 25 mmHg
    VALSALVA_ACTIVE   // espirazione in corso
};

static ValsalvaPhase currentValsalvaPhase = VALSALVA_IDLE;
static uint32_t valsalvaEndTimer = 0;  // ultimo istante con pressione >= 10 mmHg

void resetValsalvaPhase() {
    currentValsalvaPhase = VALSALVA_IDLE;
}

void updateValsalva() {
    float pressure = readValsalvaPressure();
    writeOut2Normalized(pressure / VALSALVA_OUT2_FULL_SCALE_MMHG);

    switch (currentValsalvaPhase) {
        case VALSALVA_IDLE:
            if (pressure >= VALSALVA_START_THRESH) {
                currentValsalvaPhase = VALSALVA_ACTIVE;
                valsalvaEndTimer = millis();
                restartTempo2();
                Serial.println(">> VALSALVA: Inizio Espirazione (>25 mmHg) -> Reset & Avvio T2 | Marker 2.0V (1s)");
                triggerMarker(VOLT_MARKER_VALSALVA, DURATION_MARKER_START);
            }
            break;

        case VALSALVA_ACTIVE:
            if (pressure < VALSALVA_END_THRESH) {
                if (millis() - valsalvaEndTimer >= VALSALVA_END_HOLD_MS) {
                    currentValsalvaPhase = VALSALVA_IDLE;
                    freezeTempo2Excluding(VALSALVA_END_HOLD_MS);

                    Serial.print(">> VALSALVA: Fine Espirazione -> T2 Finale Congelato: ");
                    Serial.print(getTempo2Seconds(), 2);
                    Serial.println(" s | Marker 2.0V (2s)");

                    triggerMarker(VOLT_MARKER_VALSALVA, DURATION_MARKER_END);
                    setTestMode(TEST_MODE_IDLE);
                }
            } else {
                valsalvaEndTimer = millis();
            }
            break;
    }
}
