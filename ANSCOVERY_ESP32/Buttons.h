// =====================================================================
// Buttons.h - Pulsanti MARKER e AZZERA T1
// =====================================================================

#ifndef ANSCOVERY_BUTTONS_H
#define ANSCOVERY_BUTTONS_H

#include "Config.h"

// Gesti riconosciuti sul pulsante MARKER
enum MarkerEvent {
    EVENT_NONE,
    EVENT_SINGLE_CLICK,
    EVENT_DOUBLE_CLICK,
    EVENT_LONG_PRESS
};

// Ingressi dei pulsanti con pull-up
void initButtons();

// true solo nel ciclo in cui AZZERA T1 passa da rilasciato a premuto
bool isBtnAzzeraT1Pressed();

// Da chiamare a ogni loop: restituisce un gesto solo nel ciclo in cui viene riconosciuto
MarkerEvent checkMarkerBtnEvent();

#endif
