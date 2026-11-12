#include "Hardware.h"
#include <Wire.h>
#include <math.h>

// ==========================================
// SENSORI
// ==========================================

static SensorData data = {0, 0, false, 0, 0, 0};

// Storico dell'angolo per calcolare la velocita' su 1 s
static const int TILT_HISTORY_LEN = 1000 / SENSOR_SAMPLE_MS;
static float tiltHistory[TILT_HISTORY_LEN];
static int tiltHistoryIdx = 0;
static bool tiltFilterInit = false;

static float hgBaseline = 0.0f;
static bool hgBaselineInit = false;
static bool hgBaselineTracking = true;

static bool readMpuAngle(float &angleDeg) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B); // ACCEL_XOUT_H
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)MPU6050_ADDR, (uint8_t)6) != 6) return false;

    int16_t accX = (Wire.read() << 8) | Wire.read();
    int16_t accY = (Wire.read() << 8) | Wire.read();
    int16_t accZ = (Wire.read() << 8) | Wire.read();

    float ax = accX / 16384.0f;
    float ay = accY / 16384.0f;
    float az = accZ / 16384.0f;

    // Inclinazione dell'asse Y rispetto al piano orizzontale
    float angleRad = atan2f(ay, sqrtf(ax * ax + az * az));
    angleDeg = fabsf(angleRad * (180.0f / (float)M_PI));
    return true;
}

static float readAdcAveraged(int pin) {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLE; i++) {
        sum += analogRead(pin);
    }
    return (float)sum / ADC_OVERSAMPLE;
}

static float adcToMmHg(float raw, float zeroAdc, float maxAdc, float maxMmHg) {
    float p = ((raw - zeroAdc) / (maxAdc - zeroAdc)) * maxMmHg;
    return (p < 0.0f) ? 0.0f : p;
}

void initHardware() {
    // GPIO34..39 non hanno pull-up interni: servono resistenze esterne (R1/R2/R3 sul PCB)
    pinMode(PIN_BTN_MARKER, INPUT_PULLUP);
    pinMode(PIN_BTN_AZZERA_T1, INPUT_PULLUP);

    analogReadResolution(12);
    analogSetPinAttenuation(PIN_ADC_VALSALVA, ADC_11db);
    analogSetPinAttenuation(PIN_ADC_HANDGRIP, ADC_11db);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000);

    // Risveglio MPU-6050 (PWR_MGMT_1 = 0)
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    data.tiltOk = (Wire.endTransmission(true) == 0);
    if (!data.tiltOk) {
        LOGF("!! MPU-6050 non risponde all'indirizzo 0x%02X: Tilt Test disabilitato\n", MPU6050_ADDR);
    }
}

void setHandgripBaselineTracking(bool enabled) {
    hgBaselineTracking = enabled;
}

void sampleSensors() {
    static uint32_t lastSample = 0;
    uint32_t now = millis();
    if (now - lastSample < SENSOR_SAMPLE_MS) return;
    lastSample = now;

    // --- Angolo lettino ---
    float angle;
    bool ok = readMpuAngle(angle);
    if (ok != data.tiltOk) {
        LOGF(ok ? ">> MPU-6050 tornato disponibile\n" : "!! MPU-6050: lettura fallita\n");
        data.tiltOk = ok;
    }
    if (ok) {
        if (!tiltFilterInit) {
            data.tiltAngle = angle;
            for (int i = 0; i < TILT_HISTORY_LEN; i++) tiltHistory[i] = angle;
            tiltFilterInit = true;
        } else {
            data.tiltAngle += TILT_FILTER_ALPHA * (angle - data.tiltAngle);
        }
    }
    // tiltHistory[idx] contiene il campione di 1 s fa
    data.tiltRate = data.tiltAngle - tiltHistory[tiltHistoryIdx];
    tiltHistory[tiltHistoryIdx] = data.tiltAngle;
    tiltHistoryIdx = (tiltHistoryIdx + 1) % TILT_HISTORY_LEN;

    // --- Valsalva ---
    data.valsalvaMmHg = adcToMmHg(readAdcAveraged(PIN_ADC_VALSALVA),
                                  VALSALVA_ZERO_ADC, VALSALVA_MAX_ADC, VALSALVA_MAX_MMHG);

    // --- Hand Grip: pressione assoluta e incremento sul precarico ---
    float hg = adcToMmHg(readAdcAveraged(PIN_ADC_HANDGRIP),
                         HANDGRIP_ZERO_ADC, HANDGRIP_MAX_ADC, HANDGRIP_MAX_MMHG);
    data.handgripMmHg = hg;
    if (!hgBaselineInit) {
        hgBaseline = hg;
        hgBaselineInit = true;
    }
    float delta = hg - hgBaseline;
    // Il precarico viene inseguito lentamente solo in IDLE e lontano da una stretta
    if (hgBaselineTracking && delta < HG_SQUEEZE_END_DELTA) {
        hgBaseline += HG_BASELINE_ALPHA * (hg - hgBaseline);
        delta = hg - hgBaseline;
    }
    data.handgripDelta = (delta < 0.0f) ? 0.0f : delta;
}

const SensorData& sensors() {
    return data;
}

// ==========================================
// PULSANTI (con antirimbalzo)
// ==========================================

struct DebouncedButton {
    uint8_t pin;
    bool stableState;      // HIGH = rilasciato
    bool lastRaw;
    uint32_t lastChange;
};

static DebouncedButton btnT1     = {PIN_BTN_AZZERA_T1, HIGH, HIGH, 0};
static DebouncedButton btnMarker = {PIN_BTN_MARKER,    HIGH, HIGH, 0};

// Aggiorna lo stato; ritorna true se lo stato stabile e' cambiato
static bool updateButton(DebouncedButton &b) {
    bool raw = digitalRead(b.pin);
    uint32_t now = millis();
    if (raw != b.lastRaw) {
        b.lastRaw = raw;
        b.lastChange = now;
    }
    if (raw != b.stableState && (now - b.lastChange) >= BTN_DEBOUNCE_MS) {
        b.stableState = raw;
        return true;
    }
    return false;
}

bool isBtnAzzeraT1Pressed() {
    return updateButton(btnT1) && btnT1.stableState == LOW;
}

MarkerEvent checkMarkerBtnEvent() {
    static uint32_t pressStart = 0;
    static uint32_t lastClickRelease = 0;
    static uint8_t clickCount = 0;
    static bool longPressFired = false;

    uint32_t now = millis();
    bool changed = updateButton(btnMarker);
    bool pressed = (btnMarker.stableState == LOW);

    if (changed && pressed) {
        pressStart = now;
        longPressFired = false;
    }

    // Pressione prolungata: evento generato mentre il tasto e' ancora premuto
    if (pressed && !longPressFired && (now - pressStart) >= BTN_LONG_PRESS_MS) {
        longPressFired = true;
        clickCount = 0;
        return EVENT_LONG_PRESS;
    }

    if (changed && !pressed) {
        uint32_t duration = now - pressStart;
        if (!longPressFired && duration < BTN_CLICK_MAX_MS) {
            clickCount++;
            lastClickRelease = now;
            if (clickCount >= 2) {
                clickCount = 0;
                return EVENT_DOUBLE_CLICK;
            }
        }
    }

    if (clickCount == 1 && !pressed && (now - lastClickRelease) > BTN_DOUBLE_CLICK_MS) {
        clickCount = 0;
        return EVENT_SINGLE_CLICK;
    }

    return EVENT_NONE;
}
