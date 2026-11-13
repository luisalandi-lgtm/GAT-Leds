// =====================================================================
// ANSCOVERY ESP32 - Scheda di controllo del kit per i test autonomici
// SparkBio - uso esclusivo di ricerca (RUO)
//
// Struttura del progetto:
//   Config.h          tutti i parametri (pin, soglie, tempi, tarature)
//   FSM               riconoscimento e gestione della manovra attiva
//   TiltTest, Valsalva, Handgrip, DeepBreathing   le singole manovre
//   Sensors           angolo lettino, pressione Valsalva, forza Hand Grip
//   Buttons           pulsanti MARKER e AZZERA T1
//   Outputs           uscite DAC: OUT1 marker, OUT2 segnale continuo
//   Timers            Tempo 1 e Tempo 2
//   Leds, Display     barra LED e display dei tempi
// =====================================================================

#include "Config.h"
#include "Buttons.h"
#include "Sensors.h"
#include "Outputs.h"
#include "Timers.h"
#include "FSM.h"
#include "Display.h"
#include "Leds.h"

// Eseguita una volta all'accensione o dopo un reset (tasto RST)
void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println("ANSCOVERY ESP32 - Avvio Sistema...");

    initButtons();
    initSensors();
    initLeds();
    initDisplays();
    clearLeds();
    initFSM();
}

// Ripetuta di continuo: le funzioni chiamate qui non devono bloccare a lungo
void loop() {
    // Pulsanti, sensori, manovra in corso, barra LED e marker
    updateFSM();

    // Tempo 1 e Tempo 2 sui display (MM:SS)
    updateDisplays((uint32_t)getTempo1Seconds(), (uint32_t)getTempo2Seconds());

    // Valori di OUT1 e OUT2 sul Serial Monitor
    printOutputs();
}
