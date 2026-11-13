// =====================================================================
// Handgrip.cpp - Hand Grip
// OUT2 = forza in continuo (0..HANDGRIP_OUT2_FULL_SCALE_KG).
//   HG_CALIB_WAIT_STRIKE -> HG_CALIB_IN_STRIKE   forza >= 5 kg: inizia una stretta
//   HG_CALIB_IN_STRIKE   -> registra il picco    forza < 5 kg: fine stretta; dopo la terza
//                                                calcola media e target (media / 3), T2 riparte (riposo)
//   HG_CALIB_DONE        -> HG_ISOMETRIC_ACTIVE  forza >= target: marker 3 V / 1 s, T2 riparte
//   HG_ISOMETRIC_ACTIVE  -> IDLE                 forza < target per 5 s: marker 3 V / 2 s,
//                                                T2 fermo sulla durata della contrazione
// =====================================================================

#include "Maneuvers.h"
#include "FSM.h"
#include "Sensors.h"
#include "Outputs.h"
#include "Timers.h"
#include "Leds.h"

enum HandgripState {
    HG_CALIB_WAIT_STRIKE, // in attesa di una stretta di taratura
    HG_CALIB_IN_STRIKE,   // stretta in corso (ricerca del picco)
    HG_CALIB_DONE,        // taratura completata, riposo (T2 in conteggio)
    HG_ISOMETRIC_ACTIVE   // forza sopra il target (T2 in conteggio, barra LED attiva)
};

static HandgripState currentHgState = HG_CALIB_WAIT_STRIKE;

// Taratura
static float hgPeaks[HANDGRIP_CALIB_SQUEEZES] = {0.0f, 0.0f, 0.0f};
static uint8_t hgPeakCount = 0;
static float currentMaxPeak = 0.0f;    // picco della stretta in corso

// Risultato della taratura
static float hgMediaPeaks = 0.0f;      // fondo scala della barra LED
static float hgTargetForce = 0.0f;     // media / 3

static uint32_t hgReleaseTimer = 0;    // ultimo istante con forza >= target

void resetHandgripCalib() {
    currentHgState = HG_CALIB_WAIT_STRIKE;
    hgPeakCount = 0;
    for (int i = 0; i < HANDGRIP_CALIB_SQUEEZES; i++) hgPeaks[i] = 0.0f;
    hgMediaPeaks = 0.0f;
    hgTargetForce = 0.0f;
    currentMaxPeak = 0.0f;
    Serial.println(">> HANDGRIP: Calibrazione resettata.");
}

float getHandgripTarget() {
    return hgTargetForce;
}

// Chiude una stretta di taratura; dopo l'ultima calcola media e target
static void registerCalibrationPeak() {
    hgPeaks[hgPeakCount] = currentMaxPeak;
    hgPeakCount++;

    Serial.print(">> HANDGRIP: Picco #");
    Serial.print(hgPeakCount);
    Serial.print(" registrato: ");
    Serial.print(currentMaxPeak, 1);
    Serial.println(" kg");

    if (hgPeakCount < HANDGRIP_CALIB_SQUEEZES) {
        currentHgState = HG_CALIB_WAIT_STRIKE;
        return;
    }

    float sum = 0.0f;
    for (int i = 0; i < HANDGRIP_CALIB_SQUEEZES; i++) sum += hgPeaks[i];
    hgMediaPeaks = sum / (float)HANDGRIP_CALIB_SQUEEZES;
    hgTargetForce = hgMediaPeaks / HANDGRIP_TARGET_DIVISOR;

    Serial.println("----------------------------------------");
    Serial.print(">> CALIBRAZIONE COMPLETATA!");
    Serial.print(" Media (FondoScala LED): ");
    Serial.print(hgMediaPeaks, 1);
    Serial.print(" kg | Target (1/3 Media): ");
    Serial.print(hgTargetForce, 1);
    Serial.println(" kg");
    Serial.println("----------------------------------------");

    // Inizio del riposo
    restartTempo2ClearFrozen();
    currentHgState = HG_CALIB_DONE;
}

void updateHandgrip() {
    float force = readHandgripForce();
    writeOut2Normalized(force / HANDGRIP_OUT2_FULL_SCALE_KG);

    switch (currentHgState) {
        case HG_CALIB_WAIT_STRIKE:
            if (force >= HANDGRIP_PEAK_THRESH) {
                currentHgState = HG_CALIB_IN_STRIKE;
                currentMaxPeak = force;
                Serial.print(">> HANDGRIP: Inizio Stretta #");
                Serial.println(hgPeakCount + 1);
            }
            break;

        case HG_CALIB_IN_STRIKE:
            if (force > currentMaxPeak) {
                currentMaxPeak = force;
            }
            if (force < HANDGRIP_PEAK_THRESH) {
                registerCalibrationPeak();
            }
            break;

        case HG_CALIB_DONE:
            if (force >= hgTargetForce) {
                currentHgState = HG_ISOMETRIC_ACTIVE;
                hgReleaseTimer = millis();
                restartTempo2ClearFrozen();
                Serial.println(">> HANDGRIP: Superato Target -> Reset T2 | Marker 3.0V (1s)");
                triggerMarker(VOLT_MARKER_HANDGRIP, DURATION_MARKER_START);
            }
            break;

        case HG_ISOMETRIC_ACTIVE:
            if (force < hgTargetForce) {
                if (millis() - hgReleaseTimer >= HANDGRIP_REST_HOLD_MS) {
                    freezeTempo2Excluding(HANDGRIP_REST_HOLD_MS);

                    Serial.print(">> HANDGRIP: Rilascio > 5s -> FINE TEST! T2 Finale: ");
                    Serial.print(getTempo2Seconds(), 1);
                    Serial.println(" s | Marker 3.0V (2s)");

                    triggerMarker(VOLT_MARKER_HANDGRIP, DURATION_MARKER_END);

                    updateHandgripLeds(0, 0, 0);
                    currentHgState = HG_CALIB_WAIT_STRIKE;
                    hgPeakCount = 0;

                    setTestMode(TEST_MODE_IDLE);
                }
            } else {
                hgReleaseTimer = millis();
            }
            break;
    }
}

void refreshHandgripLeds() {
    if (currentHgState == HG_CALIB_DONE || currentHgState == HG_ISOMETRIC_ACTIVE) {
        updateHandgripLeds(readHandgripForce(), hgMediaPeaks, hgTargetForce);
    }
}
