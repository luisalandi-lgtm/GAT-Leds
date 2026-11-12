#include <math.h>
#include "FSM.h"
#include "Hardware.h"
#include "Leds.h"
#include "Outputs.h"

// ==========================================
// TEMPI (Tempo 1 = test, Tempo 2 = fase)
// ==========================================

struct PhaseTimer {
    uint32_t startMs;
    uint32_t frozenMs;
    bool running;
};

static PhaseTimer tempo1 = {0, 0, false};
static PhaseTimer tempo2 = {0, 0, false};

static void timerRestart(PhaseTimer &t, uint32_t now) {
    t.startMs = now;
    t.frozenMs = 0;
    t.running = true;
}

// Ferma il timer mostrando la durata fino all'istante endMs
static void timerFreezeAt(PhaseTimer &t, uint32_t endMs) {
    if (!t.running) return;
    t.frozenMs = (endMs > t.startMs) ? endMs - t.startMs : 0;
    t.running = false;
}

static void timerZero(PhaseTimer &t) {
    t.frozenMs = 0;
    t.running = false;
}

static uint32_t timerMs(const PhaseTimer &t) {
    return t.running ? millis() - t.startMs : t.frozenMs;
}

uint32_t getTempo1Seconds() { return timerMs(tempo1) / 1000; }
uint32_t getTempo2Seconds() { return timerMs(tempo2) / 1000; }

// ==========================================
// STATO GENERALE
// ==========================================

static SystemTestMode currentMode = TEST_MODE_IDLE;
static uint32_t modeStartMs = 0;

SystemTestMode getTestMode() { return currentMode; }

static const char* modeName(SystemTestMode m) {
    switch (m) {
        case TEST_MODE_TILT:           return "TILT TEST";
        case TEST_MODE_DEEP_BREATHING: return "DEEP BREATHING";
        case TEST_MODE_VALSALVA:       return "VALSALVA";
        case TEST_MODE_HANDGRIP:       return "HAND GRIP";
        case TEST_MODE_COLD_FACE:      return "COLD FACE";
        default:                       return "IDLE";
    }
}

static void enterMode(SystemTestMode mode) {
    currentMode = mode;
    modeStartMs = millis();
    // Il precarico del manicotto si aggiorna solo a sistema fermo
    setHandgripBaselineTracking(mode == TEST_MODE_IDLE);

    switch (mode) {
        case TEST_MODE_TILT:           startConfirmBlink(255, 255, 0); break; // giallo
        case TEST_MODE_DEEP_BREATHING: startConfirmBlink(255, 0, 255); break; // viola
        case TEST_MODE_VALSALVA:       startConfirmBlink(0, 0, 255);   break; // blu
        case TEST_MODE_HANDGRIP:       startConfirmBlink(0, 255, 0);   break; // verde
        case TEST_MODE_COLD_FACE:      startConfirmBlink(0, 255, 255); break; // ciano
        default: break;
    }
    LOGF(">> FSM: %s\n", modeName(mode));
}

static void returnToIdle() {
    setOut2(0.0f);
    enterMode(TEST_MODE_IDLE);
}

// Interruzione manuale (doppio click): nessun marker, T2 congelato
static void abortTest() {
    LOGF(">> %s: interrotto dall'operatore (doppio click)\n", modeName(currentMode));
    timerFreezeAt(tempo2, millis());
    returnToIdle();
}

// ==========================================
// TILT TEST
// ==========================================

enum TiltPhase { TILT_RISING, TILT_PLATEAU, TILT_FALLING };
static TiltPhase tiltPhase = TILT_RISING;
static float tiltPeak = 0.0f;
static uint32_t tiltStableSince = 0;

// true se l'angolo e' fermo da almeno TILT_STABLE_TIME_MS
static bool tiltIsStable(float rate, uint32_t now) {
    if (fabsf(rate) < TILT_STABLE_RATE_DEG_S) {
        if (tiltStableSince == 0) tiltStableSince = now;
        return (now - tiltStableSince) >= TILT_STABLE_TIME_MS;
    }
    tiltStableSince = 0;
    return false;
}

static void startTilt(const SensorData &s, uint32_t now) {
    enterMode(TEST_MODE_TILT);
    tiltPhase = TILT_RISING;
    tiltPeak = s.tiltAngle;
    tiltStableSince = 0;
    timerRestart(tempo2, now);
    queueMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
    LOGF(">> TILT: inizio salita (%.1f gradi) -> marker 1 V / 1 s, azzera T2\n", s.tiltAngle);
}

static void startTiltDescent(float angle, uint32_t now) {
    tiltPhase = TILT_FALLING;
    tiltStableSince = 0;
    timerRestart(tempo2, now);
    queueMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
    LOGF(">> TILT: inizio discesa (%.1f gradi) -> marker 1 V / 1 s, azzera T2\n", angle);
}

static void updateTilt(const SensorData &s, uint32_t now) {
    if (s.tiltAngle > tiltPeak) tiltPeak = s.tiltAngle;
    bool falling = (s.tiltAngle < tiltPeak - TILT_FALL_DELTA_DEG) &&
                   (s.tiltRate < -TILT_MOVING_RATE_DEG_S);

    switch (tiltPhase) {
        case TILT_RISING:
            if (tiltIsStable(s.tiltRate, now)) {
                tiltPhase = TILT_PLATEAU;
                queueMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                LOGF(">> TILT: picco raggiunto (%.1f gradi) -> marker 1 V / 2 s\n", tiltPeak);
            } else if (falling) {
                // Discesa senza una fase stabile: si marca comunque il picco
                queueMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                LOGF(">> TILT: picco (%.1f gradi) non stabile -> marker 1 V / 2 s\n", tiltPeak);
                startTiltDescent(s.tiltAngle, now);
            }
            break;

        case TILT_PLATEAU:
            if (falling) startTiltDescent(s.tiltAngle, now);
            break;

        case TILT_FALLING:
            if (s.tiltAngle < TILT_START_THRESH_DEG && tiltIsStable(s.tiltRate, now)) {
                timerFreezeAt(tempo2, tiltStableSince);
                queueMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                LOGF(">> TILT: fine discesa (%.1f gradi) -> marker 1 V / 2 s\n", s.tiltAngle);
                returnToIdle();
            }
            break;
    }

    if (currentMode == TEST_MODE_TILT) {
        setOut2(s.tiltAngle / TILT_OUT2_FULL_DEG * OUT_FULL_SCALE_V);
    }
}

// ==========================================
// VALSALVA
// ==========================================

static uint32_t valsalvaBelowSince = 0;

static void startValsalva(const SensorData &s, uint32_t now) {
    enterMode(TEST_MODE_VALSALVA);
    valsalvaBelowSince = 0;
    timerRestart(tempo2, now);
    queueMarker(VOLT_MARKER_VALSALVA, DURATION_MARKER_START);
    LOGF(">> VALSALVA: inizio (%.1f mmHg) -> marker 2 V / 1 s, azzera T2\n", s.valsalvaMmHg);
}

static void updateValsalva(const SensorData &s, uint32_t now) {
    setOut2(s.valsalvaMmHg / VALSALVA_OUT2_FULL_MMHG * OUT_FULL_SCALE_V);

    if (s.valsalvaMmHg < VALSALVA_END_THRESH) {
        if (valsalvaBelowSince == 0) valsalvaBelowSince = now;
        if (now - valsalvaBelowSince >= VALSALVA_END_HOLD_MS) {
            // T2 mostra la durata dell'espirazione (fino al calo sotto soglia)
            timerFreezeAt(tempo2, valsalvaBelowSince);
            queueMarker(VOLT_MARKER_VALSALVA, DURATION_MARKER_END);
            LOGF(">> VALSALVA: fine, durata %.1f s -> marker 2 V / 2 s\n",
                 timerMs(tempo2) / 1000.0f);
            returnToIdle();
        }
    } else {
        valsalvaBelowSince = 0;
    }
}

// ==========================================
// HAND GRIP
// ==========================================

enum HandgripState {
    HG_CALIB_IN_SQUEEZE,  // stretta di taratura in corso
    HG_CALIB_WAIT,        // attesa della stretta successiva
    HG_REST,              // taratura completata, riposo (T2 avviato)
    HG_SUSTAINED          // contrazione sostenuta sopra il target
};

static HandgripState hgState = HG_CALIB_IN_SQUEEZE;
static float hgPeaks[HG_CALIB_SQUEEZES];
static uint8_t hgPeakCount = 0;
static float hgCurrentPeak = 0.0f;
static float hgMean = 0.0f;
static float hgTarget = 0.0f;
static uint32_t hgLastActivity = 0;
static uint32_t hgBelowSince = 0;

static void startHandgrip(const SensorData &s, uint32_t now) {
    enterMode(TEST_MODE_HANDGRIP);
    // La stretta che ha attivato il test e' la prima di taratura
    hgState = HG_CALIB_IN_SQUEEZE;
    hgPeakCount = 0;
    hgCurrentPeak = s.handgripDelta;
    hgMean = 0.0f;
    hgTarget = 0.0f;
    hgLastActivity = now;
    LOGF(">> HAND GRIP: stretta di taratura 1/%d\n", HG_CALIB_SQUEEZES);
}

static void updateHandgrip(const SensorData &s, uint32_t now) {
    float d = s.handgripDelta;
    setOut2(s.handgripMmHg / HANDGRIP_OUT2_FULL_MMHG * OUT_FULL_SCALE_V);

    switch (hgState) {
        case HG_CALIB_IN_SQUEEZE:
            if (d > hgCurrentPeak) hgCurrentPeak = d;
            if (d < HG_SQUEEZE_END_DELTA) {
                hgPeaks[hgPeakCount++] = hgCurrentPeak;
                hgLastActivity = now;
                LOGF(">> HAND GRIP: picco %d = %.1f mmHg sopra il precarico\n",
                     hgPeakCount, hgCurrentPeak);

                if (hgPeakCount < HG_CALIB_SQUEEZES) {
                    hgState = HG_CALIB_WAIT;
                } else {
                    float sum = 0.0f;
                    for (int i = 0; i < HG_CALIB_SQUEEZES; i++) sum += hgPeaks[i];
                    hgMean = sum / HG_CALIB_SQUEEZES;
                    hgTarget = hgMean / 3.0f;
                    if (hgTarget < HG_MIN_TARGET_DELTA) hgTarget = HG_MIN_TARGET_DELTA;
                    timerRestart(tempo2, now);
                    hgState = HG_REST;
                    LOGF(">> HAND GRIP: taratura completata. Media %.1f mmHg, target %.1f mmHg. Riposo (T2 avviato)\n",
                         hgMean, hgTarget);
                }
            }
            break;

        case HG_CALIB_WAIT:
            if (d >= HG_SQUEEZE_START_DELTA) {
                hgState = HG_CALIB_IN_SQUEEZE;
                hgCurrentPeak = d;
                LOGF(">> HAND GRIP: stretta di taratura %d/%d\n", hgPeakCount + 1, HG_CALIB_SQUEEZES);
            } else if (now - hgLastActivity >= HG_CALIB_TIMEOUT_MS) {
                LOGF(">> HAND GRIP: taratura incompleta (timeout) -> IDLE\n");
                returnToIdle();
            }
            break;

        case HG_REST:
            if (d >= hgTarget) {
                hgState = HG_SUSTAINED;
                hgBelowSince = 0;
                timerRestart(tempo2, now);
                queueMarker(VOLT_MARKER_HANDGRIP, DURATION_MARKER_START);
                LOGF(">> HAND GRIP: target superato -> marker 3 V / 1 s, azzera T2\n");
            }
            break;

        case HG_SUSTAINED:
            if (d < hgTarget) {
                if (hgBelowSince == 0) hgBelowSince = now;
                if (now - hgBelowSince >= HG_RELEASE_HOLD_MS) {
                    timerFreezeAt(tempo2, hgBelowSince);
                    queueMarker(VOLT_MARKER_HANDGRIP, DURATION_MARKER_END);
                    LOGF(">> HAND GRIP: rilascio, contrazione di %.1f s -> marker 3 V / 2 s\n",
                         timerMs(tempo2) / 1000.0f);
                    returnToIdle();
                }
            } else {
                hgBelowSince = 0;
            }
            break;
    }
}

// ==========================================
// DEEP BREATHING
// ==========================================

static bool dbRecording = false;
static uint32_t dbRecordStart = 0;

// Livello 0..1 del ritmo respiratorio: sale in 5 s, scende in 5 s (sinusoidale)
static float breathLevel(uint32_t now) {
    uint32_t t = (now - modeStartMs) % BREATH_CYCLE_MS;
    if (t < BREATH_IN_TIME_MS) {
        return 0.5f * (1.0f - cosf((float)M_PI * t / BREATH_IN_TIME_MS));
    }
    return 0.5f * (1.0f + cosf((float)M_PI * (t - BREATH_IN_TIME_MS) / BREATH_OUT_TIME_MS));
}

static void startDeepBreathing() {
    enterMode(TEST_MODE_DEEP_BREATHING);
    dbRecording = false;
    LOGF(">> DEEP BREATHING: familiarizzazione. Click singolo per avviare la registrazione\n");
}

static void startDeepBreathingRecording(uint32_t now) {
    dbRecording = true;
    dbRecordStart = now;
    timerRestart(tempo2, now);
    queueMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_START);
    LOGF(">> DEEP BREATHING: registrazione (%d atti) -> marker 4 V / 1 s, azzera T2\n",
         DEEP_BREATHING_ACTS);
}

static void updateDeepBreathing(uint32_t now) {
    setOut2(breathLevel(now) * OUT_FULL_SCALE_V);

    if (dbRecording && (now - dbRecordStart >= DEEP_BREATHING_TOTAL_MS)) {
        timerFreezeAt(tempo2, dbRecordStart + DEEP_BREATHING_TOTAL_MS);
        queueMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_END);
        LOGF(">> DEEP BREATHING: %d atti completati -> marker 4 V / 2 s\n", DEEP_BREATHING_ACTS);
        returnToIdle();
    }
}

// ==========================================
// COLD FACE / COLD PRESSOR
// ==========================================

static void startColdFace(uint32_t now) {
    enterMode(TEST_MODE_COLD_FACE);
    timerRestart(tempo2, now);
    setOut2(0.0f);
    queueMarker(VOLT_MARKER_COLD, DURATION_MARKER_START);
    LOGF(">> COLD FACE: inizio -> marker 5 V / 1 s, azzera T2\n");
}

static void updateColdFace(uint32_t now) {
    setOut2(0.0f);
    if (now - modeStartMs >= COLD_FACE_DURATION_MS) {
        timerZero(tempo2); // da specifica: T2 azzerato senza ripartire
        queueMarker(VOLT_MARKER_COLD, DURATION_MARKER_END);
        LOGF(">> COLD FACE: fine (%lu s) -> marker 5 V / 2 s\n",
             (unsigned long)(COLD_FACE_DURATION_MS / 1000));
        returnToIdle();
    }
}

// ==========================================
// LED E LOG
// ==========================================

static void updateLedBar(const SensorData &s, uint32_t now) {
    static uint32_t lastLedUpdate = 0;
    if (now - lastLedUpdate < 20) return; // 50 FPS
    lastLedUpdate = now;

    if (updateConfirmBlink()) return;

    switch (currentMode) {
        case TEST_MODE_DEEP_BREATHING:
            updateDeepBreathingLeds(breathLevel(now));
            break;
        case TEST_MODE_VALSALVA:
            updateValsalvaLeds(s.valsalvaMmHg);
            break;
        case TEST_MODE_HANDGRIP:
            if (hgState == HG_REST || hgState == HG_SUSTAINED) {
                updateHandgripLeds(s.handgripDelta, hgMean, hgTarget);
            } else {
                clearLeds();
            }
            break;
        default:
            clearLeds();
            break;
    }
}

static void logStatus(const SensorData &s, uint32_t now) {
#if !SERIAL_PLOTTER_MODE
    static uint32_t lastLog = 0;
    uint32_t interval = (currentMode == TEST_MODE_IDLE) ? 5 * STATUS_INTERVAL_MS : STATUS_INTERVAL_MS;
    if (now - lastLog < interval) return;
    lastLog = now;

    uint32_t t1 = getTempo1Seconds(), t2 = getTempo2Seconds();
    Serial.printf("[%s] T1 %02lu:%02lu T2 %02lu:%02lu | OUT1 %.2f V | OUT2 %.2f V | ",
                  modeName(currentMode),
                  (unsigned long)(t1 / 60), (unsigned long)(t1 % 60),
                  (unsigned long)(t2 / 60), (unsigned long)(t2 % 60),
                  getOut1Volts(), getOut2Volts());

    switch (currentMode) {
        case TEST_MODE_TILT:
            Serial.printf("angolo %.1f gradi (%.1f gradi/s)\n", s.tiltAngle, s.tiltRate);
            break;
        case TEST_MODE_VALSALVA:
            Serial.printf("SP1 %.1f mmHg\n", s.valsalvaMmHg);
            break;
        case TEST_MODE_HANDGRIP:
            Serial.printf("manicotto %.1f mmHg (+%.1f), target +%.1f\n",
                          s.handgripMmHg, s.handgripDelta, hgTarget);
            break;
        case TEST_MODE_DEEP_BREATHING:
            Serial.printf(dbRecording ? "registrazione\n" : "familiarizzazione\n");
            break;
        case TEST_MODE_COLD_FACE:
            Serial.printf("%lu s rimanenti\n",
                          (unsigned long)((COLD_FACE_DURATION_MS - (now - modeStartMs)) / 1000));
            break;
        default:
            Serial.printf("angolo %.1f%s | SP1 %.1f mmHg | manicotto %.1f mmHg (+%.1f)\n",
                          s.tiltAngle, s.tiltOk ? "" : " (MPU assente)",
                          s.valsalvaMmHg, s.handgripMmHg, s.handgripDelta);
            break;
    }
#else
    (void)s; (void)now;
#endif
}

// ==========================================
// CICLO PRINCIPALE
// ==========================================

void initFSM() {
    timerZero(tempo1);
    timerZero(tempo2);
    returnToIdle();
    LOGF("[IDLE] Trigger: Tilt >%.0f gradi in salita, Valsalva >%.0f mmHg, "
         "Hand Grip +%.0f mmHg, doppio click = Deep Breathing, pressione %d s = Cold Face\n",
         TILT_START_THRESH_DEG, VALSALVA_START_THRESH, HG_SQUEEZE_START_DELTA,
         BTN_LONG_PRESS_MS / 1000);
}

static void dispatchIdle(const SensorData &s, MarkerEvent ev, uint32_t now) {
    switch (ev) {
        case EVENT_DOUBLE_CLICK: startDeepBreathing(); return;
        case EVENT_LONG_PRESS:   startColdFace(now);  return;
        case EVENT_SINGLE_CLICK:
            timerRestart(tempo2, now);
            queueMarker(VOLT_MARKER_GENERIC, DURATION_MARKER_START);
            LOGF(">> MARKER: click singolo -> marker generico 1.5 V / 1 s, azzera T2\n");
            return;
        default: break;
    }

    if (s.valsalvaMmHg >= VALSALVA_START_THRESH) {
        startValsalva(s, now);
    } else if (s.handgripDelta >= HG_SQUEEZE_START_DELTA) {
        startHandgrip(s, now);
    } else if (s.tiltOk && s.tiltAngle >= TILT_START_THRESH_DEG &&
               s.tiltRate >= TILT_MOVING_RATE_DEG_S) {
        // Serve un angolo in aumento: un lettino fermo (es. 20 gradi per la Valsalva) non avvia il Tilt
        startTilt(s, now);
    }
}

static void handleEventInTest(MarkerEvent ev, uint32_t now) {
    if (ev == EVENT_NONE) return;

    if (ev == EVENT_DOUBLE_CLICK) {
        if (currentMode == TEST_MODE_DEEP_BREATHING && now - modeStartMs < DB_CLICK_LOCKOUT_MS) return;
        abortTest();
        return;
    }

    if (ev == EVENT_SINGLE_CLICK) {
        if (currentMode == TEST_MODE_DEEP_BREATHING) {
            if (!dbRecording && now - modeStartMs >= DB_CLICK_LOCKOUT_MS) {
                startDeepBreathingRecording(now);
            }
        } else {
            // Marker generico durante un test: non tocca T2 della manovra
            queueMarker(VOLT_MARKER_GENERIC, DURATION_MARKER_START);
            LOGF(">> MARKER: click singolo -> marker generico 1.5 V / 1 s\n");
        }
    }
    // EVENT_LONG_PRESS durante un test: ignorato
}

void updateFSM() {
    sampleSensors();
    const SensorData &s = sensors();
    uint32_t now = millis();

    if (isBtnAzzeraT1Pressed()) {
        timerRestart(tempo1, now);
        LOGF(">> T1: azzerato\n");
    }

    MarkerEvent ev = checkMarkerBtnEvent();

    if (currentMode == TEST_MODE_IDLE) {
        dispatchIdle(s, ev, now);
    } else {
        handleEventInTest(ev, now);
    }

    switch (currentMode) {
        case TEST_MODE_TILT:           updateTilt(s, now);          break;
        case TEST_MODE_VALSALVA:       updateValsalva(s, now);      break;
        case TEST_MODE_HANDGRIP:       updateHandgrip(s, now);      break;
        case TEST_MODE_DEEP_BREATHING: updateDeepBreathing(now);    break;
        case TEST_MODE_COLD_FACE:      updateColdFace(now);         break;
        default:                       setOut2(0.0f);               break;
    }

    updateLedBar(s, now);
    logStatus(s, now);
    updateOutputs();
}
