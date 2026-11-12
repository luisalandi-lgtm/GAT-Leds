// =====================================================================
// FSM.cpp - Macchina a stati del sistema
// In IDLE riconosce quale manovra sta iniziando (dai sensori o dal
// pulsante MARKER), poi ne gestisce le fasi: marker su OUT1, segnale
// continuo su OUT2, Tempo 1 / Tempo 2 e barra LED.
// Ogni manovra ha la sua sotto-macchina a stati (un enum e uno switch).
// =====================================================================

#include "FSM.h"
#include "Hardware.h"
#include "Leds.h"
#include "Display.h"

// Dichiarazione anticipata: la funzione e' definita in fondo al file
void logCurrentTestStatus();
// Manovra attiva (TEST_MODE_IDLE = nessuna)
SystemTestMode currentTestMode = TEST_MODE_IDLE;

// Soglie per Auto-Detect in IDLE
#define THRESHOLD_TRIGGER_TILT     10.0f // Angolo > 10° attiva il Tilt
#define THRESHOLD_TRIGGER_VALSALVA 20.0f // Pressione SP1 > 20 mmHg attiva il Valsalva
#define THRESHOLD_TRIGGER_HANDGRIP  5.0f // Forza SP2 > 5 Kg attiva l'Handgrip

// Stati specifici per il Deep Breathing
static bool deepBreathingLedsActive = false; // Barra LED attiva per familiarizzazione
static bool deepBreathingRecording  = false; // Registrazione effettiva in corso (10 atti)
// Istante del doppio click che ha avviato il Deep Breathing
static uint32_t dbTestStartTime = 0; 
const uint32_t HUMAN_LOCKOUT_MS = 1500; // 1.5 secondi di blocco minimo dopo l'avvio

// Durata di 10 atti respiratori (10 cicli da 10s = 100s = 1 minuto e 40 secondi)
#define DEEP_BREATHING_TOTAL_MS (10 * BREATH_CYCLE_MS)

// Stati per la FSM Valsalva
// VALSALVA_IDLE: attesa del superamento di 25 mmHg; VALSALVA_ACTIVE: espirazione in corso
enum ValsalvaPhase {
    VALSALVA_IDLE,
    VALSALVA_ACTIVE
};

static ValsalvaPhase currentValsalvaPhase = VALSALVA_IDLE;
// Ultimo istante con pressione >= 10 mmHg: serve a contare i 3 s sotto soglia
static uint32_t valsalvaEndTimer = 0;

// --- STATI TILT TEST ---
enum TiltPhase {
    TILT_IDLE,       // Orizzontale
    TILT_RISING,     // Salita in corso
    TILT_VERTICAL,   // Verticale stabile (T2 attivo)
    TILT_FALLING     // Discesa in corso
};

static TiltPhase currentTiltPhase = TILT_IDLE;
// Azzerata in initFSM() ma non usata altrove
static float lastTiltAngle = 0.0f;
// Istante da cui l'angolo e' stabile (per i 2 s di attesa)
static uint32_t tiltStabilityTimer = 0;
// Non usata nel codice attuale
static float peakVerticalAngle = 0.0f; 
// Angolo al controllo precedente (1 s prima): serve per la velocita'
static float angleOneSecondAgo = 0.0f; 

// --- STATI FSM HANDGRIP ---
enum HandgripState {
    HG_CALIB_WAIT_STRIKE, // In attesa di una stretta di calibrazione
    HG_CALIB_IN_STRIKE,   // Stretta in corso (tracciamento picco)
    HG_CALIB_DONE,        // 3 picchi registrati, in riposo (T2 in scorrimento)
    HG_ISOMETRIC_ACTIVE   // Forza > Target (T2 azzerato e avviato, LED attivi)
};

static HandgripState currentHgState = HG_CALIB_WAIT_STRIKE;

// Calibrazione (3 strette)
static float hgPeaks[3] = {0.0f, 0.0f, 0.0f};
static uint8_t hgPeakCount = 0;
// Picco della stretta in corso
static float currentMaxPeak = 0.0f;

// Target e Fondoscala
static float hgMediaPeaks = 0.0f;  // Fondo scala Barra LED
static float hgTargetForce = 0.0f; // Target = Media / 3

// Ultimo istante con forza >= target: serve a contare i 5 s di rilascio
static uint32_t hgReleaseTimer = 0;

// Gestione dei tempi (in millisecondi)
// StartMs = istante di partenza, Running = sta contando, FrozenMs = valore mostrato da fermo.
// Tempo 1 = test (pulsante AZZERA T1), Tempo 2 = fase della manovra.
uint32_t tempo1StartMs = 0;
bool tempo1Running = false;
uint32_t tempo1FrozenMs = 0;

uint32_t tempo2StartMs = 0;
bool tempo2Running = false;
uint32_t tempo2FrozenMs = 0; 

// Controllo temporale a 50 FPS per il refresh dei LED
static uint32_t lastLedUpdate = 0;

// Cambia la manovra attiva. Azzera le sotto-macchine di Valsalva, Tilt e Hand Grip,
// spegne i LED e, per una manovra (non per IDLE), fa lampeggiare la barra del suo colore.
// Il lampeggio usa delay(): blocca il programma per circa 3,2 s (2 x (480 + 100 + 1000) ms).
void setTestMode(SystemTestMode newMode) {
    // Nessun effetto se la manovra e' gia' quella attiva
    if (currentTestMode == newMode) return;
    
    currentTestMode = newMode;
    
    // AZZERAMENTO FORZATO
    currentValsalvaPhase = VALSALVA_IDLE;
    currentTiltPhase = TILT_IDLE;
    resetHandgripCalib();

    clearLeds();
    
    uint8_t r = 0, g = 0, b = 0;
    bool executeWait = true;

    switch (newMode) {
        case TEST_MODE_TILT:
            Serial.println(">> AUTO-DETECT: Attivato TILT TEST! Attesa 2s...");
            r = 255; g = 255; b = 0; // GIALLO
            break;

        case TEST_MODE_DEEP_BREATHING:
            Serial.println(">> AUTO-DETECT: Attivato DEEP BREATHING! Attesa 2s...");
            r = 255; g = 0; b = 255; // VIOLA
            break;

        case TEST_MODE_VALSALVA:
            Serial.println(">> AUTO-DETECT: Attivato VALSALVA! Attesa 2s...");
            r = 0; g = 0; b = 255; // BLU
            break;

        case TEST_MODE_HANDGRIP:
            Serial.println(">> AUTO-DETECT: Attivato HANDGRIP! Attesa 2s...");
            resetHandgripCalib();
            r = 0; g = 255; b = 0; // VERDE
            break;

        case TEST_MODE_IDLE:
        default:
            Serial.println(">> FSM: Sistema in IDLE");
            clearLeds();
            executeWait = false;
            break;
    }

    // Lampeggio di conferma ripetuto 2 volte, ciascuno seguito da 1 s di pausa
    if (executeWait) {
        for (int i = 0; i < 2; i++) {
            blinkLedsConfirm(r, g, b); 
            delay(100);

            clearLeds(); 
            delay(1000);
        }
        Serial.println(">> Fine attesa 2s: Inizio effettivo del test!");
    }
}

// Stato iniziale all'accensione: tempi a zero, nessuna manovra attiva
void initFSM() {
    tempo1StartMs = 0;
    tempo1Running = false;
    tempo1FrozenMs = 0;
    tempo2StartMs = 0;
    tempo2Running = false;
    tempo2FrozenMs = 0;
    deepBreathingLedsActive = false;
    deepBreathingRecording = false;
    currentTiltPhase = TILT_IDLE;
    lastTiltAngle = 0.0f;
    currentTestMode = TEST_MODE_IDLE;
}

// Pulsante AZZERA T1: se T1 e' fermo lo fa ripartire da 0, se sta contando lo ferma
void onAzzeraTempo1Pressed() {
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

// ---------------------------------------------------------------------
// TILT TEST
// OUT2 = angolo del lettino in continuo (0..TILT_ANGLE_MAX -> DAC 0..255).
// Le fasi vengono valutate una volta al secondo:
//   TILT_IDLE     -> TILT_RISING    angolo >= 30 e in salita: marker 1 V / 1 s
//   TILT_RISING   -> TILT_VERTICAL  angolo >= 55 fermo per 2 s: marker 1 V / 2 s, T2 riparte
//   TILT_VERTICAL -> TILT_FALLING   angolo < 55: marker 1 V / 1 s, T2 si ferma
//   TILT_FALLING  -> IDLE           angolo < 5 fermo per 2 s: marker 1 V / 2 s
// ---------------------------------------------------------------------
void updateTiltFSM() {
    if (currentTestMode != TEST_MODE_TILT) return;

    float angle = readTiltAngle();
    float normalized = angle / TILT_ANGLE_MAX;
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;
    writeOutput(PIN_DAC_OUT2, (uint8_t)(normalized * 255.0f));

    // Da qui in poi il codice gira solo una volta al secondo
    static uint32_t lastTiltCheckTime = 0;
    if (millis() - lastTiltCheckTime < 1000) {
        return; 
    }
    lastTiltCheckTime = millis();

    // Variazione dell'angolo nell'ultimo secondo (gradi/s): > 0 salita, < 0 discesa
    float deltaAnglePerSec = angle - angleOneSecondAgo;

    switch (currentTiltPhase) {
        case TILT_IDLE:
            if (angle >= THRESHOLD_TILT_DEG && deltaAnglePerSec > 0.5f) {
                currentTiltPhase = TILT_RISING;
                tiltStabilityTimer = millis();
                Serial.println(">> TILT: Inizio Salita (>30°) -> Marker 1.0V (1s)");
                triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
            }
            break;

        case TILT_RISING:
            if (angle >= 55.0f && abs(deltaAnglePerSec) < 0.5f) {
                if (millis() - tiltStabilityTimer >= 2000) {
                    currentTiltPhase = TILT_VERTICAL;
                    tempo2StartMs = millis();
                    tempo2Running = true;

                    Serial.println(">> TILT: Posizione Verticale Raggiunta (>=55°) -> Marker 1.0V (2s) & AVVIO T2");
                    triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_END);
                }
            } else {
                tiltStabilityTimer = millis();
            }
            break;

        case TILT_VERTICAL:
            if (angle < 55.0f) {
                currentTiltPhase = TILT_FALLING;
                tiltStabilityTimer = millis();

                tempo2Running = false;
                tempo2StartMs = 0;

                Serial.println(">> TILT: Inizio Discesa (<55°) -> Marker 1.0V (1s) & STOP T2");
                triggerMarker(VOLT_MARKER_TILT, DURATION_MARKER_START);
            }
            break;

        case TILT_FALLING:
            if (angle < 5.0f && abs(deltaAnglePerSec) < 0.5f) {
                if (millis() - tiltStabilityTimer >= 2000) {
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

    // Memorizza l'angolo per il confronto del secondo successivo
    angleOneSecondAgo = angle;
}

// ---------------------------------------------------------------------
// VALSALVA
// OUT2 = pressione SP1 (0..VALSALVA_MAX_MMHG -> DAC 0..255).
//   VALSALVA_IDLE   -> VALSALVA_ACTIVE  pressione >= 25 mmHg: marker 2 V / 1 s, T2 riparte
//   VALSALVA_ACTIVE -> IDLE             pressione < 10 mmHg per 3 s: marker 2 V / 2 s,
//                                       T2 fermo sulla durata del soffio (3 s esclusi)
// ---------------------------------------------------------------------
void updateValsalvaFSM() {
    if (currentTestMode != TEST_MODE_VALSALVA) return;

    float pressure = readValsalvaPressure(); 

    float normalized = pressure / VALSALVA_MAX_MMHG;
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;
    writeOutput(PIN_DAC_OUT2, (uint8_t)(normalized * 255.0f));

    switch (currentValsalvaPhase) {
        case VALSALVA_IDLE:
            if (pressure >= VALSALVA_START_THRESH) {
                currentValsalvaPhase = VALSALVA_ACTIVE;
                valsalvaEndTimer = millis();
                tempo2StartMs = millis(); 
                tempo2Running = true;

                Serial.println(">> VALSALVA: Inizio Espirazione (>25 mmHg) -> Reset & Avvio T2 | Marker 2.0V (1s)");
                triggerMarker(VOLT_MARKER_VALSALVA, DURATION_MARKER_START);
            }
            break;

        case VALSALVA_ACTIVE:
            if (pressure < VALSALVA_END_THRESH) {
                if (millis() - valsalvaEndTimer >= VALSALVA_END_HOLD_MS) {
                    currentValsalvaPhase = VALSALVA_IDLE;
                    tempo2FrozenMs = millis() - tempo2StartMs - VALSALVA_END_HOLD_MS; 
                    tempo2Running = false;
                    tempo2StartMs = 0;

                    Serial.print(">> VALSALVA: Fine Espirazione -> T2 Finale Congelato: ");
                    Serial.print(tempo2FrozenMs / 1000.0f, 2);
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

// Riporta l'Hand Grip all'inizio della taratura (nessuna stretta registrata)
void resetHandgripCalib() {
    currentHgState = HG_CALIB_WAIT_STRIKE;
    hgPeakCount = 0;
    hgPeaks[0] = hgPeaks[1] = hgPeaks[2] = 0.0f;
    hgMediaPeaks = 0.0f;
    hgTargetForce = 0.0f;
    currentMaxPeak = 0.0f;
    Serial.println(">> HANDGRIP: Calibrazione resettata.");
}

// ---------------------------------------------------------------------
// HAND GRIP
// OUT2 = forza (0..HANDGRIP_ADC_MAX_KG -> DAC 0..255).
//   HG_CALIB_WAIT_STRIKE -> HG_CALIB_IN_STRIKE   forza >= 5 kg: inizia una stretta
//   HG_CALIB_IN_STRIKE   -> registra il picco    forza < 5 kg: fine stretta; dopo la terza
//                                                calcola media e target (media / 3), T2 riparte (riposo)
//   HG_CALIB_DONE        -> HG_ISOMETRIC_ACTIVE  forza >= target: marker 3 V / 1 s, T2 riparte
//   HG_ISOMETRIC_ACTIVE  -> IDLE                 forza < target per 5 s: marker 3 V / 2 s,
//                                                T2 fermo sulla durata della contrazione
// ---------------------------------------------------------------------
void updateHandgripFSM() {
    if (currentTestMode != TEST_MODE_HANDGRIP) return;

    float force = readHandGripPressure(); 
    float normalizedOut2 = force / HANDGRIP_ADC_MAX_KG;
    if (normalizedOut2 > 1.0f) normalizedOut2 = 1.0f;
    if (normalizedOut2 < 0.0f) normalizedOut2 = 0.0f;
    writeOutput(PIN_DAC_OUT2, (uint8_t)(normalizedOut2 * 255.0f));

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
                hgPeaks[hgPeakCount] = currentMaxPeak;
                hgPeakCount++;

                Serial.print(">> HANDGRIP: Picco #");
                Serial.print(hgPeakCount);
                Serial.print(" registrato: ");
                Serial.print(currentMaxPeak, 1);
                Serial.println(" kg");

                if (hgPeakCount < 3) {
                    currentHgState = HG_CALIB_WAIT_STRIKE;
                } else {
                    hgMediaPeaks = (hgPeaks[0] + hgPeaks[1] + hgPeaks[2]) / 3.0f;
                    hgTargetForce = hgMediaPeaks / 3.0f; 

                    Serial.println("----------------------------------------");
                    Serial.print(">> CALIBRAZIONE COMPLETATA!");
                    Serial.print(" Media (FondoScala LED): ");
                    Serial.print(hgMediaPeaks, 1);
                    Serial.print(" kg | Target (1/3 Media): ");
                    Serial.print(hgTargetForce, 1);
                    Serial.println(" kg");
                    Serial.println("----------------------------------------");

                    tempo2StartMs = millis();
                    tempo2Running = true;
                    tempo2FrozenMs = 0;

                    currentHgState = HG_CALIB_DONE;
                }
            }
            break;

        case HG_CALIB_DONE:
            if (force >= hgTargetForce) {
                currentHgState = HG_ISOMETRIC_ACTIVE;
                hgReleaseTimer = millis();

                tempo2StartMs = millis();
                tempo2Running = true;
                tempo2FrozenMs = 0;

                Serial.println(">> HANDGRIP: Superato Target -> Reset T2 | Marker 3.0V (1s)");
                triggerMarker(VOLT_MARKER_HANDGRIP, DURATION_MARKER_START);
            }
            break;

        case HG_ISOMETRIC_ACTIVE:
            if (force < hgTargetForce) {
                if (millis() - hgReleaseTimer >= HANDGRIP_REST_HOLD_MS) {
                    tempo2FrozenMs = millis() - tempo2StartMs - HANDGRIP_REST_HOLD_MS;
                    tempo2Running = false;
                    tempo2StartMs = 0;

                    Serial.print(">> HANDGRIP: Rilascio > 5s -> FINE TEST! T2 Finale: ");
                    Serial.print(tempo2FrozenMs / 1000.0f, 1);
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

// ---------------------------------------------------------------------
// CICLO DELLA MACCHINA A STATI (chiamato a ogni loop)
// 1. pulsante AZZERA T1
// 2. evento del pulsante MARKER (click singolo, doppio, pressione lunga)
// 3. in IDLE: riconoscimento della manovra (doppio click, Tilt, Valsalva, Hand Grip)
// 4. effetto dell'evento MARKER nella manovra corrente
// 5. avanzamento della manovra attiva
// 6. barra LED (al massimo ogni 20 ms), riga di stato su seriale, fine del marker
// ---------------------------------------------------------------------
void updateFSM() {
    if (isBtnAzzeraT1Pressed()) {
        onAzzeraTempo1Pressed();
    }

    MarkerEvent btnEvent = checkMarkerBtnEvent();

    // AUTO-DETECTION IN STATO IDLE
    if (currentTestMode == TEST_MODE_IDLE) {
        float currentAngle = readTiltAngle();
        float valsalvaP = readValsalvaPressure();
        float handgripF = readHandGripPressure();

        if (btnEvent == EVENT_DOUBLE_CLICK) {
            dbTestStartTime = millis(); 
            setTestMode(TEST_MODE_DEEP_BREATHING);
            deepBreathingLedsActive = true;
            deepBreathingRecording = false;
            btnEvent = EVENT_NONE; 
        }
        else if (abs(currentAngle) > THRESHOLD_TRIGGER_TILT) {
            setTestMode(TEST_MODE_TILT);
        }
        else if (valsalvaP >= THRESHOLD_TRIGGER_VALSALVA) {
            setTestMode(TEST_MODE_VALSALVA);
        }
        else if (handgripF >= THRESHOLD_TRIGGER_HANDGRIP) {
            setTestMode(TEST_MODE_HANDGRIP);
        }
    }

    // GESTIONE EVENTI TASTO MARKER
    switch (btnEvent) {
        // Click singolo: nel Deep Breathing avvia la registrazione,
        // altrove marker generico 1,5 V / 1 s e T2 riparte
        case EVENT_SINGLE_CLICK:
            if (currentTestMode == TEST_MODE_DEEP_BREATHING) {
                if (millis() - dbTestStartTime >= HUMAN_LOCKOUT_MS) {
                    if (deepBreathingLedsActive && !deepBreathingRecording) {
                        deepBreathingRecording = true;
                        tempo2StartMs = millis();
                        tempo2Running = true;

                        Serial.println(">> DEEP BREATHING: Click -> Inizio Registrazione (10 atti / 1m40s) - Marker 4.0V (1s)");
                        triggerMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_START);
                    }
                }
            } 
            else if (currentTestMode != TEST_MODE_DEEP_BREATHING) {
                tempo2StartMs = millis();
                tempo2Running = true;
                Serial.println(">> MARKER: Single Click -> Marker Generico (1.5V) & Azzera T2");
                triggerMarker(VOLT_MARKER_GENERIC, DURATION_MARKER_START);
            }
            break;

        // Doppio click nel Deep Breathing: stop senza marker (l'avvio e' gestito sopra, in IDLE)
        case EVENT_DOUBLE_CLICK:
            if (currentTestMode == TEST_MODE_DEEP_BREATHING) {
                if (millis() - dbTestStartTime >= HUMAN_LOCKOUT_MS) {
                    deepBreathingLedsActive = false;
                    deepBreathingRecording = false;
                    tempo2Running = false;
                    tempo2StartMs = 0;
                    clearLeds();
                    Serial.println(">> DEEP BREATHING: Doppio Click -> Stop Manuale");
                    setTestMode(TEST_MODE_IDLE);
                }
            }
            break;

        // Pressione >= 5 s (riconosciuta al rilascio): marker Cold Face 5 V / 1 s, T2 riparte.
        // Non cambia la manovra attiva.
        case EVENT_LONG_PRESS:
            tempo2StartMs = millis();
            tempo2Running = true;
            Serial.println(">> MARKER: Long Press -> Cold Face (5.0V) & Azzera T2");
            triggerMarker(VOLT_MARKER_COLD, DURATION_MARKER_START);
            break;

        default:
            break;
    }

    // ESECUZIONE LOGICA FSM
    switch (currentTestMode) {
        case TEST_MODE_TILT:
            updateTiltFSM();
            break;

        case TEST_MODE_DEEP_BREATHING:
            // Deep Breathing: dopo 100 s di registrazione marker 4 V / 2 s e ritorno in IDLE
            if (deepBreathingRecording) {
                uint32_t elapsedTime = millis() - tempo2StartMs;
                if (elapsedTime >= DEEP_BREATHING_TOTAL_MS) {
                    deepBreathingRecording = false;
                    deepBreathingLedsActive = false;
                    tempo2Running = false;
                    tempo2StartMs = 0;

                    Serial.println(">> DEEP BREATHING: 10 Atti Completati -> Marker Fine 4.0V (2s) e Stop LED");
                    triggerMarker(VOLT_MARKER_DEEPBREATH, DURATION_MARKER_END);
                    setTestMode(TEST_MODE_IDLE);
                }
            }
            break;

        case TEST_MODE_VALSALVA:
            updateValsalvaFSM();
            break;

        case TEST_MODE_HANDGRIP:
            updateHandgripFSM();
            break;

        case TEST_MODE_IDLE:
        default:
            break;
    }

    // --- GESTIONE LED LIMITATA A 50 FPS (20ms) ---
    if (millis() - lastLedUpdate >= 20) {
        lastLedUpdate = millis();

        switch (currentTestMode) {
            case TEST_MODE_IDLE:
                updateIdleLeds();
                break;

            case TEST_MODE_DEEP_BREATHING:
                updateDeepBreathingLeds(deepBreathingLedsActive);
                break;

            case TEST_MODE_VALSALVA:
                updateValsalvaLeds(readValsalvaPressure());
                break;

            case TEST_MODE_HANDGRIP:
                if (currentHgState == HG_CALIB_DONE || currentHgState == HG_ISOMETRIC_ACTIVE) {
                    updateHandgripLeds(readHandGripPressure(), hgMediaPeaks, hgTargetForce);
                }
                break;

            case TEST_MODE_TILT:
            default:
                break;
        }
    }

    // Riga di stato su seriale (una volta al secondo durante un test)
    logCurrentTestStatus();
    // Riporta OUT1 a 0 V quando la durata del marker e' trascorsa
    updateMarker();
}

// Tempi in secondi per i display: valore corrente se il tempo sta contando,
// altrimenti il valore congelato
float getTempo1Seconds() {
    if (tempo1Running) {
        return (millis() - tempo1StartMs) / 1000.0f;
    } else {
        return tempo1FrozenMs / 1000.0f;
    }
}

// true mentre la barra LED del Deep Breathing e' attiva
bool isDeepBreathingActive() {
    return deepBreathingLedsActive;
}

float getTempo2Seconds() {
    if (tempo2Running) {
        return (millis() - tempo2StartMs) / 1000.0f;
    } else {
        return tempo2FrozenMs / 1000.0f;
    }
}

// In IDLE stampa un messaggio una sola volta; durante un test stampa ogni secondo
// il valore del sensore della manovra.
void logCurrentTestStatus() {
    static SystemTestMode lastLoggedMode = (SystemTestMode)-1; 
    static uint32_t lastLogTime = 0;

    if (currentTestMode == TEST_MODE_IDLE) {
        if (lastLoggedMode != TEST_MODE_IDLE) {
            Serial.println("[STATO: IDLE] Attesa test (Tilt >10°, Valsalva >20mmHg, Handgrip >5kg, DblClick DB)");
            lastLoggedMode = TEST_MODE_IDLE;
        }
        return; 
    }

    if (millis() - lastLogTime >= 1000 || lastLoggedMode != currentTestMode) {
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
                Serial.print(readHandGripPressure(), 1);
                Serial.print(" kg | Target: ");
                Serial.print(hgTargetForce, 1);
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