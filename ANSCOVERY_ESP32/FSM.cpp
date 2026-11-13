// =====================================================================
// FSM.cpp - Macchina a stati del sistema
// In IDLE riconosce quale manovra sta iniziando (dai sensori o dal
// pulsante MARKER) e la avvia; poi a ogni loop fa avanzare la manovra
// attiva, aggiorna la barra LED e stampa lo stato su seriale.
// La logica di ogni manovra e' nel suo file (TiltTest, Valsalva,
// Handgrip, DeepBreathing).
// =====================================================================

#include "FSM.h"
#include "Maneuvers.h"
#include "Sensors.h"
#include "Buttons.h"
#include "Outputs.h"
#include "Timers.h"
#include "Leds.h"

static SystemTestMode currentTestMode = TEST_MODE_IDLE;
static uint32_t lastLedUpdate = 0;

static void logCurrentTestStatus();

// ---------------------------------------------------------------------
// Cambio di manovra
// Azzera le fasi di Valsalva, Tilt e Hand Grip e spegne i LED. Per una
// manovra (non per IDLE) fa lampeggiare la barra del suo colore: il
// lampeggio usa delay() e blocca il programma per circa 3,2 s.
// ---------------------------------------------------------------------
void setTestMode(SystemTestMode newMode) {
    if (currentTestMode == newMode) return;

    currentTestMode = newMode;

    resetValsalvaPhase();
    resetTiltPhase();
    resetHandgripCalib();

    clearLeds();

    uint8_t r = 0, g = 0, b = 0;
    bool executeWait = true;

    switch (newMode) {
        case TEST_MODE_TILT:
            Serial.println(">> AUTO-DETECT: Attivato TILT TEST! Attesa 2s...");
            r = 255; g = 255; b = 0;     // giallo
            break;

        case TEST_MODE_DEEP_BREATHING:
            Serial.println(">> AUTO-DETECT: Attivato DEEP BREATHING! Attesa 2s...");
            r = 255; g = 0; b = 255;     // viola
            break;

        case TEST_MODE_VALSALVA:
            Serial.println(">> AUTO-DETECT: Attivato VALSALVA! Attesa 2s...");
            r = 0; g = 0; b = 255;       // blu
            break;

        case TEST_MODE_HANDGRIP:
            Serial.println(">> AUTO-DETECT: Attivato HANDGRIP! Attesa 2s...");
            resetHandgripCalib();
            r = 0; g = 255; b = 0;       // verde
            break;

        case TEST_MODE_IDLE:
        default:
            Serial.println(">> FSM: Sistema in IDLE");
            clearLeds();
            executeWait = false;
            break;
    }

    if (executeWait) {
        for (int i = 0; i < CONFIRM_REPEATS; i++) {
            blinkLedsConfirm(r, g, b);
            delay(CONFIRM_GAP_MS);

            clearLeds();
            delay(CONFIRM_PAUSE_MS);
        }
        Serial.println(">> Fine attesa 2s: Inizio effettivo del test!");
    }
}

void initFSM() {
    initTimers();
    resetDeepBreathing();
    resetTiltPhase();
    currentTestMode = TEST_MODE_IDLE;
}

// ---------------------------------------------------------------------
// In IDLE: avvia la prima manovra riconosciuta, in quest'ordine:
// doppio click -> Deep Breathing, angolo -> Tilt, pressione -> Valsalva,
// forza -> Hand Grip. Restituisce l'evento del pulsante ancora da gestire
// (EVENT_NONE se il doppio click e' stato usato per avviare il Deep Breathing).
// ---------------------------------------------------------------------
static MarkerEvent detectTestStart(MarkerEvent btnEvent) {
    float currentAngle = readTiltAngle();
    float valsalvaP = readValsalvaPressure();
    float handgripF = readHandgripForce();

    if (btnEvent == EVENT_DOUBLE_CLICK) {
        startDeepBreathing();
        btnEvent = EVENT_NONE;
    }
    else if (abs(currentAngle) > IDLE_TRIGGER_TILT_DEG) {
        setTestMode(TEST_MODE_TILT);
    }
    else if (valsalvaP >= IDLE_TRIGGER_VALSALVA_MMHG) {
        setTestMode(TEST_MODE_VALSALVA);
    }
    else if (handgripF >= IDLE_TRIGGER_HANDGRIP_KG) {
        setTestMode(TEST_MODE_HANDGRIP);
    }
    return btnEvent;
}

// ---------------------------------------------------------------------
// Effetto dei gesti sul pulsante MARKER
// ---------------------------------------------------------------------
static void handleMarkerEvent(MarkerEvent btnEvent) {
    switch (btnEvent) {
        case EVENT_SINGLE_CLICK:
            if (currentTestMode == TEST_MODE_DEEP_BREATHING) {
                deepBreathingOnClick();
            } else {
                // Marker generico, in qualunque altro stato
                restartTempo2();
                Serial.println(">> MARKER: Single Click -> Marker Generico (1.5V) & Azzera T2");
                triggerMarker(VOLT_MARKER_GENERIC, DURATION_MARKER_START);
            }
            break;

        case EVENT_DOUBLE_CLICK:
            // In IDLE il doppio click avvia il Deep Breathing (detectTestStart)
            if (currentTestMode == TEST_MODE_DEEP_BREATHING) {
                deepBreathingOnDoubleClick();
            }
            break;

        case EVENT_LONG_PRESS:
            // Cold Face: solo marker, la manovra attiva non cambia
            restartTempo2();
            Serial.println(">> MARKER: Long Press -> Cold Face (5.0V) & Azzera T2");
            triggerMarker(VOLT_MARKER_COLD, DURATION_MARKER_START);
            break;

        default:
            break;
    }
}

static void runActiveTest() {
    switch (currentTestMode) {
        case TEST_MODE_TILT:           updateTiltTest();      break;
        case TEST_MODE_DEEP_BREATHING: updateDeepBreathing(); break;
        case TEST_MODE_VALSALVA:       updateValsalva();      break;
        case TEST_MODE_HANDGRIP:       updateHandgrip();      break;
        case TEST_MODE_IDLE:
        default:
            break;
    }
}

// Barra LED, al massimo ogni LED_REFRESH_MS (spenta durante il Tilt)
static void refreshLeds() {
    if (millis() - lastLedUpdate < LED_REFRESH_MS) return;
    lastLedUpdate = millis();

    switch (currentTestMode) {
        case TEST_MODE_IDLE:           updateIdleLeds();                                   break;
        case TEST_MODE_DEEP_BREATHING: updateDeepBreathingLeds(isDeepBreathingLedsActive()); break;
        case TEST_MODE_VALSALVA:       updateValsalvaLeds(readValsalvaPressure());         break;
        case TEST_MODE_HANDGRIP:       refreshHandgripLeds();                              break;
        case TEST_MODE_TILT:
        default:
            break;
    }
}

// ---------------------------------------------------------------------
// Ciclo della macchina a stati (chiamato a ogni loop)
// ---------------------------------------------------------------------
void updateFSM() {
    if (isBtnAzzeraT1Pressed()) {
        toggleTempo1();
    }

    MarkerEvent btnEvent = checkMarkerBtnEvent();

    if (currentTestMode == TEST_MODE_IDLE) {
        btnEvent = detectTestStart(btnEvent);
    }

    handleMarkerEvent(btnEvent);
    runActiveTest();
    refreshLeds();
    logCurrentTestStatus();
    updateMarker();
}

// ---------------------------------------------------------------------
// Stato su seriale: in IDLE un messaggio una sola volta, durante un test
// una riga ogni STATUS_LOG_INTERVAL_MS (e subito al cambio di manovra)
// ---------------------------------------------------------------------
static void logCurrentTestStatus() {
    static SystemTestMode lastLoggedMode = (SystemTestMode)-1;
    static uint32_t lastLogTime = 0;

    if (currentTestMode == TEST_MODE_IDLE) {
        if (lastLoggedMode != TEST_MODE_IDLE) {
            Serial.println("[STATO: IDLE] Attesa test (Tilt >10°, Valsalva >20mmHg, Handgrip >5kg, DblClick DB)");
            lastLoggedMode = TEST_MODE_IDLE;
        }
        return;
    }

    if (millis() - lastLogTime >= STATUS_LOG_INTERVAL_MS || lastLoggedMode != currentTestMode) {
        lastLogTime = millis();
        lastLoggedMode = currentTestMode;

        switch (currentTestMode) {
            case TEST_MODE_TILT:
                Serial.print("[STATO: TILT TEST] Angolo lettino: ");
                Serial.print(readTiltAngle(), 1);
                Serial.println(" °");
                break;

            case TEST_MODE_VALSALVA:
                Serial.print("[STATO: VALSALVA] Pressione SP1: ");
                Serial.print(readValsalvaPressure(), 1);
                Serial.println(" mmHg");
                break;

            case TEST_MODE_HANDGRIP:
                Serial.print("[STATO: HANDGRIP] Forza SP2: ");
                Serial.print(readHandgripForce(), 1);
                Serial.print(" kg | Target: ");
                Serial.print(getHandgripTarget(), 1);
                Serial.println(" kg");
                break;

            case TEST_MODE_DEEP_BREATHING:
                Serial.print("[STATO: DEEP BREATHING] T2: ");
                Serial.print(getTempo2Seconds(), 1);
                Serial.println(" s / 100 s");
                break;

            default:
                break;
        }
    }
}
