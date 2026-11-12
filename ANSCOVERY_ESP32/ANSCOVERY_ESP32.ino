// =====================================================================
// ANSCOVERY ESP32 - Scheda di controllo del kit per i test autonomici
// SparkBio - uso esclusivo di ricerca (RUO)
//
// Sketch principale: inizializza i moduli e richiama di continuo la
// macchina a stati (FSM), i display dei tempi e la stampa delle uscite.
// =====================================================================

#include "Config.h"
#include "Hardware.h"
#include "FSM.h"
#include "Display.h"
#include "Leds.h"

// Eseguita una volta all'accensione o dopo un reset (tasto RST)
void setup() {
  Serial.begin(115200);
  Serial.println("ANSCOVERY ESP32 - Avvio Sistema...");

  // Pulsanti, ingressi analogici, bus I2C del sensore angolare
  initHardware();
  // Barra LED WS2812B
  initLeds();
  // Display TM1637 di Tempo 1 e Tempo 2 (mostrano 00:00)
  initDisplayHardware();
  clearLeds();
  // Tempi a zero, nessuna manovra attiva (IDLE)
  initFSM();
}

// Ripetuta di continuo: le funzioni chiamate qui non devono bloccare a lungo
void loop() {
    // Pulsanti, sensori, manovra in corso, barra LED e marker
    updateFSM();

    // Scrive Tempo 1 e Tempo 2 sui display (MM:SS)
    updateDisplays(getTempo1Seconds(), getTempo2Seconds());

    // Stampa i valori di OUT1 e OUT2 sul Serial Monitor
    printOutputs();
}