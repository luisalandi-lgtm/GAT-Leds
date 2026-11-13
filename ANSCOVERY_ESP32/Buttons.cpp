// =====================================================================
// Buttons.cpp - Pulsanti MARKER e AZZERA T1
// I pulsanti sono attivi bassi: LOW = premuto.
// =====================================================================

#include "Buttons.h"

void initButtons() {
    pinMode(PIN_BTN_MARKER, INPUT_PULLUP);
    pinMode(PIN_BTN_AZZERA_T1, INPUT_PULLUP);
}

// Fronte di pressione, senza antirimbalzo
bool isBtnAzzeraT1Pressed() {
    static bool lastBtnState = HIGH;
    bool currentState = digitalRead(PIN_BTN_AZZERA_T1);
    bool pressed = (lastBtnState == HIGH && currentState == LOW);
    lastBtnState = currentState;
    return pressed;
}

// Gesti sul pulsante MARKER:
// - pressione >= BTN_LONG_PRESS_MS          -> EVENT_LONG_PRESS (al rilascio)
// - pressione da BTN_MIN_CLICK_MS in su     -> conta un click
// - BTN_DOUBLE_CLICK_GAP_MS senza nuovi click -> EVENT_SINGLE_CLICK (1 click)
//                                              o EVENT_DOUBLE_CLICK (2 o piu')
MarkerEvent checkMarkerBtnEvent() {
    static bool lastState = HIGH;
    static unsigned long pressStartTime = 0;
    static unsigned long lastReleaseTime = 0;
    static int clickCount = 0;
    static bool waitingForSecondClick = false;

    bool currentState = digitalRead(PIN_BTN_MARKER);
    unsigned long now = millis();
    MarkerEvent detectedEvent = EVENT_NONE;

    // Pressione
    if (lastState == HIGH && currentState == LOW) {
        pressStartTime = now;
    }

    // Rilascio: classifica la pressione in base alla durata
    if (lastState == LOW && currentState == HIGH) {
        unsigned long pressDuration = now - pressStartTime;

        if (pressDuration >= BTN_LONG_PRESS_MS) {
            detectedEvent = EVENT_LONG_PRESS;
            clickCount = 0;
            waitingForSecondClick = false;
        } else if (pressDuration > BTN_MIN_CLICK_MS) {
            clickCount++;
            lastReleaseTime = now;
            waitingForSecondClick = true;
        }
    }

    // Nessun nuovo click entro la finestra: decide tra singolo e doppio
    if (waitingForSecondClick && (now - lastReleaseTime > BTN_DOUBLE_CLICK_GAP_MS)) {
        if (clickCount == 1) {
            detectedEvent = EVENT_SINGLE_CLICK;
        } else if (clickCount >= 2) {
            detectedEvent = EVENT_DOUBLE_CLICK;
        }
        clickCount = 0;
        waitingForSecondClick = false;
    }

    lastState = currentState;
    return detectedEvent;
}
