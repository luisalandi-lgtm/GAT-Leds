// =====================================================================
// Config.h - Parametri del progetto
// Tutti i pin, le soglie delle manovre, le tensioni dei marker, i tempi
// e le tarature dei sensori. Per cambiare un valore si modifica qui.
// =====================================================================

#ifndef ANSCOVERY_CONFIG_H
#define ANSCOVERY_CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. PIN DELLA SCHEDA
// ==========================================

// Display TM1637 di Tempo 1 (CN10) e Tempo 2 (CN6)
#define PIN_TM1637_CLK1       2
#define PIN_TM1637_DIO1       4
#define PIN_TM1637_CLK2       5
#define PIN_TM1637_DIO2       18

// Barra LED WS2812B (DIN)
#define PIN_LED_BAR           15

// Pulsanti (attivi bassi)
#define PIN_BTN_MARKER        35    // pulsante 1
#define PIN_BTN_AZZERA_T1     32    // pulsante 2

// Sensori di pressione (ingressi ADC)
#define PIN_ADC_VALSALVA      12    // pressione 1
#define PIN_ADC_HANDGRIP      14    // pressione 2

// Sensore angolare MPU-6050 (I2C)
#define PIN_I2C_SDA           21
#define PIN_I2C_SCL           22

// Uscite DAC verso il poligrafo
#define PIN_DAC_OUT1          25    // OUT1 = marker
#define PIN_DAC_OUT2          26    // OUT2 = segnale continuo

// ==========================================
// 2. SERIALE, DISPLAY, BARRA LED
// ==========================================

#define SERIAL_BAUD               115200
#define STATUS_LOG_INTERVAL_MS    1000    // riga di stato durante un test
#define OUTPUTS_PRINT_INTERVAL_MS 1000    // riga [USCITE] (oltre che a ogni cambio di OUT1)

#define DISPLAY_BRIGHTNESS    0x0f        // luminosita' massima dei TM1637

#define NUM_LEDS              48          // 6 barre da 8 LED
#define LED_BRIGHTNESS        50          // 0..255
#define LED_REFRESH_MS        20          // aggiornamento barra: al massimo 50 volte al secondo

// Lampeggio di conferma all'avvio di una manovra (bloccante):
// CONFIRM_REPEATS volte: 2 lampeggi da LED_BLINK_MS acceso / LED_BLINK_MS spento,
// poi CONFIRM_GAP_MS, barra spenta, poi CONFIRM_PAUSE_MS
#define CONFIRM_REPEATS       2
#define LED_BLINK_COUNT       2
#define LED_BLINK_MS          120
#define CONFIRM_GAP_MS        100
#define CONFIRM_PAUSE_MS      1000

// ==========================================
// 3. USCITE ANALOGICHE E MARKER
// ==========================================

// Il DAC dell'ESP32 e' a 8 bit: 0..255 = 0..3,3 V.
// Senza stadio 0-5 V i marker da 4 V e 5 V escono entrambi a 3,3 V.
#define DAC_MAX_VALUE         255
#define DAC_FULL_SCALE_V      3.3f

// Livelli dei marker su OUT1
#define VOLT_MARKER_TILT        1.0f
#define VOLT_MARKER_GENERIC     1.5f   // click singolo
#define VOLT_MARKER_VALSALVA    2.0f
#define VOLT_MARKER_HANDGRIP    3.0f
#define VOLT_MARKER_DEEPBREATH  4.0f
#define VOLT_MARKER_COLD        5.0f   // pressione prolungata (Cold Face)

// Durata degli impulsi: inizio evento / fine evento
#define DURATION_MARKER_START   1000
#define DURATION_MARKER_END     2000

// ==========================================
// 4. PULSANTI
// ==========================================

#define BTN_MIN_CLICK_MS        50     // pressioni piu' brevi vengono ignorate
#define BTN_DOUBLE_CLICK_GAP_MS 300    // attesa di un altro click prima di decidere
#define BTN_LONG_PRESS_MS       5000   // pressione prolungata (riconosciuta al rilascio)

// ==========================================
// 5. RICONOSCIMENTO DELLA MANOVRA IN IDLE
// ==========================================

#define IDLE_TRIGGER_TILT_DEG        10.0f  // angolo lettino oltre 10 gradi -> Tilt
#define IDLE_TRIGGER_VALSALVA_MMHG   20.0f  // pressione boccaglio da 20 mmHg -> Valsalva
#define IDLE_TRIGGER_HANDGRIP_KG     5.0f   // forza da 5 kg -> Hand Grip

// ==========================================
// 6. TILT TEST (sensore MPU-6050)
// ==========================================

#define MPU6050_ADDR             0x68
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_LSB_PER_G        16384.0f   // scala +/- 2 g

#define TILT_CHECK_INTERVAL_MS   1000   // le fasi vengono valutate una volta al secondo
#define TILT_START_DEG           30.0f  // inizio salita
#define TILT_VERTICAL_DEG        55.0f  // posizione verticale
#define TILT_HORIZONTAL_DEG      5.0f   // ritorno in orizzontale
#define TILT_RATE_DEG_S          0.5f   // sopra: angolo in movimento; sotto: angolo fermo
#define TILT_STABLE_TIME_MS      2000   // per quanto l'angolo deve restare fermo
#define TILT_OUT2_FULL_SCALE_DEG 60.0f  // OUT2: 0..60 gradi -> DAC 0..255

// ==========================================
// 7. VALSALVA (pressione SP1, mmHg)
// ==========================================

// Conversione ADC -> mmHg (da tarare sul sensore)
#define VALSALVA_ZERO_ADC        30.0f    // ADC a 0 mmHg
#define VALSALVA_MAX_ADC         3800.0f  // ADC a VALSALVA_MAX_MMHG
#define VALSALVA_MAX_MMHG        100.0f

#define VALSALVA_START_THRESH    25.0f    // inizio espirazione
#define VALSALVA_END_THRESH      10.0f    // fine: pressione sotto questa soglia...
#define VALSALVA_END_HOLD_MS     3000     // ...per 3 s consecutivi
#define VALSALVA_OUT2_FULL_SCALE_MMHG VALSALVA_MAX_MMHG  // OUT2: 0..100 mmHg -> DAC 0..255

// Barra LED
#define VALSALVA_LED_FULL_SCALE_MMHG 60.0f
#define VALSALVA_TARGET_MMHG         40.0f
#define VALSALVA_LED_MIN_MMHG        5.0f  // sotto questa pressione barra spenta

// ==========================================
// 8. HAND GRIP (forza, kg)
// ==========================================

// Conversione ADC -> kg (da tarare sul sensore)
#define HANDGRIP_ZERO_ADC        30.0f    // ADC a 0 kg
#define HANDGRIP_MAX_ADC         3800.0f  // ADC a HANDGRIP_MAX_KG
#define HANDGRIP_MAX_KG          50.0f
#define HANDGRIP_FORCE_OFFSET_KG 5.0f     // sottratti alla forza letta

#define HANDGRIP_CALIB_SQUEEZES  3        // strette di taratura
#define HANDGRIP_PEAK_THRESH     5.0f     // sopra: stretta in corso; sotto: stretta finita
#define HANDGRIP_TARGET_DIVISOR  3.0f     // target = media dei picchi / 3
#define HANDGRIP_REST_HOLD_MS    5000     // fine prova: sotto target per 5 s
#define HANDGRIP_OUT2_FULL_SCALE_KG 50.0f // OUT2: 0..50 kg -> DAC 0..255

#define HANDGRIP_LED_MIN_KG      1.0f     // sotto questa forza barra spenta

// ==========================================
// 9. DEEP BREATHING
// ==========================================

#define BREATH_IN_TIME_MS        5000     // barra che si riempie (inspirazione)
#define BREATH_OUT_TIME_MS       5000     // barra che si svuota (espirazione)
#define BREATH_CYCLE_MS          (BREATH_IN_TIME_MS + BREATH_OUT_TIME_MS)
#define DEEP_BREATHING_BREATHS   10
#define DEEP_BREATHING_TOTAL_MS  (DEEP_BREATHING_BREATHS * BREATH_CYCLE_MS)  // 100 s
#define DEEP_BREATHING_LOCKOUT_MS 1500    // click ignorati dopo il doppio click di avvio

#endif
