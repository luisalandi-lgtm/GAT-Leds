// =====================================================================
// Sensors.h - Lettura dei sensori delle manovre
// =====================================================================

#ifndef SENSORS_H
#define SENSORS_H

#include "Config.h"

// ADC a 12 bit, bus I2C e risveglio dell'MPU-6050
void initSensors();

// Inclinazione del lettino in gradi (0..90). 0 se l'MPU-6050 non risponde.
float readTiltAngle();

// Pressione del boccaglio Valsalva in mmHg (mai negativa)
float readValsalvaPressure();

// Forza dell'Hand Grip in kg (mai negativa)
float readHandgripForce();

#endif
