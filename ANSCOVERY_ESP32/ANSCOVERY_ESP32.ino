#include "Config.h"
#include "Hardware.h"
#include "FSM.h"
#include "Display.h"
#include "Leds.h"

void setup() {
  Serial.begin(115200);
  Serial.println("ANSCOVERY ESP32 - Avvio Sistema...");

  initHardware();
  initLeds();
  initDisplayHardware();
  clearLeds();
  initFSM();
}

void loop() {
    updateFSM();

    updateDisplays(getTempo1Seconds(), getTempo2Seconds());
}