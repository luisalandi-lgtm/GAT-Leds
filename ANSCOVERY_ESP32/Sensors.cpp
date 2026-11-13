// =====================================================================
// Sensors.cpp - Lettura dei sensori
// Angolo del lettino dall'MPU-6050 (I2C), pressione Valsalva e forza
// Hand Grip dagli ingressi ADC.
// =====================================================================

#include <Wire.h>
#include "Sensors.h"

// Numero di letture ADC mediate per ridurre il rumore
#define ADC_SAMPLES 4

void initSensors() {
    // ADC a 12 bit (0..4095), attenuazione 11 dB: ingresso fino a circa 3,1 V
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_ADC_VALSALVA, ADC_11db);
    analogSetPinAttenuation(PIN_ADC_HANDGRIP, ADC_11db);

    pinMode(PIN_ADC_VALSALVA, INPUT);
    pinMode(PIN_ADC_HANDGRIP, INPUT);

    // Bus I2C e risveglio dell'MPU-6050 (registro PWR_MGMT_1 = 0)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_REG_PWR_MGMT_1);
    Wire.write(0);
    Wire.endTransmission(true);
}

float readTiltAngle() {
    // Lettura dei 6 byte di accelerazione X, Y, Z
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_REG_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 6, true);

    if (Wire.available() < 6) return 0.0f;

    int16_t accX = (Wire.read() << 8) | Wire.read();
    int16_t accY = (Wire.read() << 8) | Wire.read();
    int16_t accZ = (Wire.read() << 8) | Wire.read();

    // Accelerazioni in g
    float ax = accX / MPU6050_LSB_PER_G;
    float ay = accY / MPU6050_LSB_PER_G;
    float az = accZ / MPU6050_LSB_PER_G;

    // Inclinazione dell'asse Y rispetto al piano orizzontale, in gradi
    float angleRad = atan2(ay, sqrt(ax * ax + az * az));
    float angleDeg = abs(angleRad * (180.0f / M_PI));

    return angleDeg;
}

// Media di ADC_SAMPLES letture consecutive
static float readAdcAverage(uint8_t pin) {
    int sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(pin);
    }
    return sum / (float)ADC_SAMPLES;
}

float readValsalvaPressure() {
    float raw = readAdcAverage(PIN_ADC_VALSALVA);

    float pressure = ((raw - VALSALVA_ZERO_ADC) / (VALSALVA_MAX_ADC - VALSALVA_ZERO_ADC)) * VALSALVA_MAX_MMHG;
    if (pressure < 0.0f) pressure = 0.0f;

    return pressure;
}

float readHandgripForce() {
    float raw = readAdcAverage(PIN_ADC_HANDGRIP);

    float force = ((raw - HANDGRIP_ZERO_ADC) / (HANDGRIP_MAX_ADC - HANDGRIP_ZERO_ADC)) * HANDGRIP_MAX_KG;
    force -= HANDGRIP_FORCE_OFFSET_KG;

    if (force < 0.0f) force = 0.0f;
    return force;
}
