#include "Hardware.h"
#include "Config.h"
#include <Adafruit_NeoPixel.h> // Inclusione per la Barra LED

#define MPU6050_ADDR 0x68 // Indirizzo I2C standard dell'MPU-6050

static Adafruit_NeoPixel ledBar(NUM_LEDS, PIN_LED_BAR, NEO_GRB + NEO_KHZ800);
static unsigned long markerStartTime = 0;
static unsigned long markerDuration = 0;
static bool markerActive = false;

void initHardware() {
    pinMode(PIN_BTN_MARKER, INPUT_PULLUP);
    pinMode(PIN_BTN_AZZERA_T1, INPUT_PULLUP);
    
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_ADC_VALSALVA, ADC_11db);
    analogSetPinAttenuation(PIN_ADC_HANDGRIP, ADC_11db);

    pinMode(PIN_ADC_VALSALVA, INPUT);
    pinMode(PIN_ADC_HANDGRIP, INPUT);

    // Inizializzazione Barra LED NeoPixel
    ledBar.begin();
    ledBar.setBrightness(50); // Luminosità regolabile (0..255)

    // Inizializzazione Bus I2C per MPU-6050
    Wire.begin(21, 22); // SDA = IO21, SCL = IO22 (modifica i pin se diversi su MYSIGN)
    
    // Risveglio MPU-6050 dal registro di sleep (PWR_MGMT_1 -> 0x6B)
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B);
    Wire.write(0); 
    Wire.endTransmission(true);
}

// Calcolo effettivo dell'angolo di inclinazione (Tilt) con MPU-6050
float readTiltAngle() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B); // Registro iniziale dati accelerometro (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 6, true);

    if (Wire.available() < 6) return 0.0f;

    int16_t AccX = (Wire.read() << 8) | Wire.read();
    int16_t AccY = (Wire.read() << 8) | Wire.read();
    int16_t AccZ = (Wire.read() << 8) | Wire.read();

    // Converti le letture grezze dell'accelerometro in g (scala +/- 2g = 16384 LSB/g)
    float ax = AccX / 16384.0f;
    float ay = AccY / 16384.0f;
    float az = AccZ / 16384.0f;

    // Calcolo dell'angolo di Tilt rispetto all'asse di gravità (in Gradi)
    float angleRad = atan2(ay, sqrt(ax * ax + az * az));
    float angleDeg = abs(angleRad * (180.0f / M_PI));

    return angleDeg;
}

void updateTiltDAC(float angle) {
    // Mappa l'angolo 0°..60° sulla scala DAC 8-bit (0..255)
    float normalized = angle / 60.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;
    
    uint8_t dacVal = (uint8_t)(normalized * 255.0f);
    dacWrite(PIN_DAC_OUT2, dacVal);
}

float readValsalvaPressure() {
    // Media mobile rapida su 4 campioni per abbattere il rumore ADC dell'ESP32
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += analogRead(PIN_ADC_VALSALVA);
    }
    float raw = sum / 4.0f;

    // Calcolo della pressione in mmHg con clamp per evitare valori negativi
    float pressure = ((raw - VALSALVA_ZERO_ADC) / (VALSALVA_MAX_ADC - VALSALVA_ZERO_ADC)) * VALSALVA_MAX_MMHG;
    if (pressure < 0.0f) pressure = 0.0f;

    return pressure;
}

float readHandGripPressure() {
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += analogRead(PIN_ADC_HANDGRIP);
    }
    float raw = sum / 4.0f;

    // Calcolo della forza/pressione in Kg
    float force = ((raw - HANDGRIP_ZERO_ADC) / (HANDGRIP_MAX_ADC - HANDGRIP_ZERO_ADC)) * HANDGRIP_MAX_KG;

    force -= 5.0f;
    
    if (force < 0.0f) force = 0.0f;
    return force;
}

bool isBtnAzzeraT1Pressed() {
    static bool lastBtnState = HIGH;
    bool currentState = digitalRead(PIN_BTN_AZZERA_T1);
    bool pressed = (lastBtnState == HIGH && currentState == LOW);
    lastBtnState = currentState;
    return pressed;
}

MarkerEvent checkMarkerBtnEvent() {
    static bool lastState = HIGH;
    static unsigned long pressStartTime = 0;
    static unsigned long lastReleaseTime = 0;
    static int clickCount = 0;
    static bool waitingForSecondClick = false;

    bool currentState = digitalRead(PIN_BTN_MARKER);
    unsigned long now = millis();
    MarkerEvent detectedEvent = EVENT_NONE;

    if (lastState == HIGH && currentState == LOW) {
        pressStartTime = now;
    }

    if (lastState == LOW && currentState == HIGH) {
        unsigned long pressDuration = now - pressStartTime;

        if (pressDuration >= 5000) {
            detectedEvent = EVENT_LONG_PRESS;
            clickCount = 0;
            waitingForSecondClick = false;
        } else if (pressDuration > 50) {
            clickCount++;
            lastReleaseTime = now;
            waitingForSecondClick = true;
        }
    }

    if (waitingForSecondClick && (now - lastReleaseTime > 300)) {
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

void triggerMarker(float voltage, unsigned long durationMs) {
    if (markerActive) return;
    
    int dacValue = (int)((voltage / 3.3f) * 255.0f);
    if (dacValue > 255) dacValue = 255;
    
    dacWrite(PIN_DAC_OUT1, dacValue);
    markerStartTime = millis();
    markerDuration = durationMs;
    markerActive = true;
}

void updateMarker() {
    if (markerActive && (millis() - markerStartTime >= markerDuration)) {
        dacWrite(PIN_DAC_OUT1, 0);
        markerActive = false;
    }
}