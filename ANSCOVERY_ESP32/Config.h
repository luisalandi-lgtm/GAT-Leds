#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. MAPPATURA HARDWARE PCB
// ==========================================

// Display TM1637 #1 (CN10)
#define PIN_TM1637_CLK1       2
#define PIN_TM1637_DIO1       4

// Display TM1637 #2 (CN6)
#define PIN_TM1637_CLK2       5
#define PIN_TM1637_DIO2       18

// Striscia/Barra LED WS2812B (NeoPixel)
#define PIN_LED_BAR     15      // Pin GPIO collegato al DIN della barra LED
#define NUM_LEDS        48      // Numero totale di LED
#define LED_BRIGHTNESS  50      // Luminosità (0-255)

// Pulsanti
#define PIN_BTN1              35
#define PIN_BTN2              32
#define PIN_BTN_MARKER        PIN_BTN1
#define PIN_BTN_AZZERA_T1     PIN_BTN2

// Sensori e Ingressi Analogici
#define PIN_PRESS1            12
#define PIN_PRESS2            14
#define PIN_ADC_TILT          34
#define PIN_ADC_VALSALVA      PIN_PRESS1
#define PIN_ADC_HANDGRIP      PIN_PRESS2

// Uscite DAC per Marker
#define PIN_DAC1              25
#define PIN_DAC2              26
#define PIN_DAC_OUT1          PIN_DAC1
#define PIN_DAC_OUT2          PIN_DAC2

// ==========================================
// 2. SOGLIE E PARAMETRI CLINICI (FSM)
// ==========================================

// Soglie di Attivazione
#define THRESHOLD_TILT_DEG    30.0f
#define TILT_ANGLE_MAX   60.0f  // Fondo scala per la mappatura dell'angolo (0..60 gradi)
#define VALSALVA_START_THRESH    25.0f
#define VALSALVA_END_THRESH   10.0f
#define PRELOAD_HANDGRIP      15.0f

// Tensioni Marker (DAC)
#define VOLT_MARKER_TILT      1.0f
#define VOLT_MARKER_VALSALVA  2.0f
#define VOLT_MARKER_HANDGRIP  3.0f
#define VOLT_MARKER_DEEPBREATH 4.0f
#define VOLT_MARKER_COLD      5.0f
#define VOLT_MARKER_GENERIC      1.5f  // 1 Click: Marker generico (1.5V)

// Durate Marker (ms)
#define DURATION_MARKER_START 1000
#define DURATION_MARKER_END   2000

// Sensore Valsalva (Pressione Polmonare - mmHg)
#define VALSALVA_ZERO_ADC    30.0f  // Valore ADC a riposo (da tarare)
#define VALSALVA_MAX_ADC    3800.0f  // Valore ADC a fondo scala (es. 100 mmHg)
#define VALSALVA_MAX_MMHG    100.0f  // Fondo scala in mmHg
#define VALSALVA_MAX_SCALE 60.0f  // Fondo scala della barra LED (60 mmHg)
#define TARGET_VALSALVA    40.0f  // Target da raggiungere (40 mmHg)
#define VALSALVA_END_HOLD_MS 3000 // 3 secondi continuous hold < 10 mmHg

// --- PARAMETRI HANDGRIP TEST ---
#define HANDGRIP_PEAK_THRESH       5.0f    // Soglia minima per considerare una "stretta" (kg)
#define HANDGRIP_REST_HOLD_MS      5000    // Tempo di rilascio finale per terminare il test (5 sec)
#define VOLT_MARKER_HANDGRIP       3.0f    // Marker OUT1 = 3.0 V (1s inizio, 2s fine)
#define HANDGRIP_ADC_MAX_KG        50.0f   // Fondo scala fisico del sensore di forza

// Sensore Handgrip (Forza Isometrica - Kg / mmHg)
#define HANDGRIP_ZERO_ADC    30.0f  // Valore ADC a riposo (da tarare)
#define HANDGRIP_MAX_ADC    3800.0f  // Valore ADC a fondo scala (es. 50 kg)
#define HANDGRIP_MAX_KG       50.0f  // Fondo scala in Kg

// Soglie Cliniche
#define THRESHOLD_VALSALVA   40.0f   // Target tipico Valsalva: 40 mmHg
#define PRELOAD_HANDGRIP      5.0f   // Precarico/forza minima per avvio Handgrip

// Parametri Ritmici Deep Breathing
#define BREATH_IN_TIME_MS   5000  // 5 secondi Inspirazione
#define BREATH_OUT_TIME_MS  5000  // 5 secondi Espirazione
#define BREATH_CYCLE_MS     (BREATH_IN_TIME_MS + BREATH_OUT_TIME_MS) // 10000 ms totali

#endif