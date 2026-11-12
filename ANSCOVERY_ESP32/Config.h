#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. MAPPATURA HARDWARE PCB
// ==========================================

// Display TM1637 #1 (CN10) - Tempo 1
#define PIN_TM1637_CLK1       2
#define PIN_TM1637_DIO1       4

// Display TM1637 #2 (CN6) - Tempo 2
#define PIN_TM1637_CLK2       5
#define PIN_TM1637_DIO2       18

// Barra LED WS2812B: 6 barre da 8 LED in serie
#define PIN_LED_BAR           15
#define NUM_LEDS              48
#define LED_BRIGHTNESS        50      // 0..255

// Pulsanti (attivi bassi)
#define PIN_BTN1              35
#define PIN_BTN2              32
#define PIN_BTN_MARKER        PIN_BTN1
#define PIN_BTN_AZZERA_T1     PIN_BTN2

// Sensori analogici
// ATTENZIONE: GPIO12 e' un pin di strapping (vedi README, sezione "Note hardware").
#define PIN_PRESS1            12
#define PIN_PRESS2            14
#define PIN_ADC_VALSALVA      PIN_PRESS1
#define PIN_ADC_HANDGRIP      PIN_PRESS2

// Sensore angolare MPU-6050 (I2C)
#define PIN_I2C_SDA           21
#define PIN_I2C_SCL           22
#define MPU6050_ADDR          0x68

// Uscite DAC
#define PIN_DAC1              25
#define PIN_DAC2              26
#define PIN_DAC_OUT1          PIN_DAC1   // OUT1 = marker
#define PIN_DAC_OUT2          PIN_DAC2   // OUT2 = segnale continuo

// ==========================================
// 2. USCITE ANALOGICHE OUT1 / OUT2
// ==========================================
// Tutti i valori di uscita nel firmware sono espressi in Volt "logici" 0..5 V,
// cioe' la tensione che arrivera' al poligrafo. Il DAC (0..3.3 V, 8 bit) viene
// scritto in proporzione: 5 V logici = DAC 255. Con lo stadio op-amp previsto
// (guadagno ~1.515) l'uscita reale coincidera' con il valore logico.
// Finche' lo stadio non c'e', la tensione reale sul pin e' valore / 1.515.
#define OUT_FULL_SCALE_V      5.0f

// Pausa a 0 V tra due marker consecutivi in coda (per distinguerli sul tracciato)
#define MARKER_GAP_MS         300

// ==========================================
// 3. OUTPUT SU SERIALE
// ==========================================
// 0 = Serial Monitor: eventi testuali + riga di stato ogni secondo con OUT1/OUT2
// 1 = Serial Plotter: solo righe "OUT1:x,OUT2:y" (nessun messaggio testuale)
#define SERIAL_PLOTTER_MODE   0
#define SERIAL_BAUD           115200
#define PLOTTER_INTERVAL_MS   50
#define STATUS_INTERVAL_MS    1000

#if SERIAL_PLOTTER_MODE
  #define LOGF(...) do {} while (0)
#else
  #define LOGF(...) Serial.printf(__VA_ARGS__)
#endif

// ==========================================
// 4. CODIFICA MARKER OUT1 (Tabella 2 del documento di analisi)
// ==========================================
#define VOLT_MARKER_TILT        1.0f
#define VOLT_MARKER_GENERIC     1.5f   // click singolo fuori dal Deep Breathing
#define VOLT_MARKER_VALSALVA    2.0f
#define VOLT_MARKER_HANDGRIP    3.0f
#define VOLT_MARKER_DEEPBREATH  4.0f
#define VOLT_MARKER_COLD        5.0f   // Cold Face / Cold Pressor

#define DURATION_MARKER_START   1000
#define DURATION_MARKER_END     2000

// ==========================================
// 5. CAMPIONAMENTO SENSORI
// ==========================================
#define SENSOR_SAMPLE_MS      20      // 50 Hz
#define ADC_OVERSAMPLE        8       // media di N letture per campione

// ==========================================
// 6. TILT TEST
// ==========================================
#define TILT_START_THRESH_DEG   30.0f  // avvio: angolo oltre 30 gradi...
#define TILT_MOVING_RATE_DEG_S  1.0f   // ...e in salita almeno a questa velocita'
#define TILT_STABLE_RATE_DEG_S  0.5f   // sotto questa velocita' l'angolo e' "stabile"
#define TILT_STABLE_TIME_MS     1500   // per quanto deve restare stabile (picco / fine)
#define TILT_FALL_DELTA_DEG     3.0f   // discesa: angolo sotto il picco di almeno N gradi
#define TILT_OUT2_FULL_DEG      90.0f  // OUT2: 0..90 gradi -> 0..5 V
#define TILT_FILTER_ALPHA       0.2f   // filtro esponenziale sull'angolo

// ==========================================
// 7. VALSALVA (SP1, mmHg)
// ==========================================
#define VALSALVA_ZERO_ADC       30.0f   // ADC a riposo (da tarare)
#define VALSALVA_MAX_ADC        3800.0f // ADC a fondo scala (da tarare)
#define VALSALVA_MAX_MMHG       100.0f  // pressione corrispondente a VALSALVA_MAX_ADC
#define VALSALVA_START_THRESH   25.0f   // avvio manovra
#define VALSALVA_END_THRESH     10.0f   // fine manovra...
#define VALSALVA_END_HOLD_MS    3000    // ...se sotto soglia per 3 s continui
#define VALSALVA_TARGET         40.0f   // target paziente (barra LED)
#define VALSALVA_LED_SCALE      60.0f   // fondo scala barra LED
#define VALSALVA_OUT2_FULL_MMHG 100.0f  // OUT2: 0..100 mmHg -> 0..5 V

// ==========================================
// 8. HAND GRIP (manicotto precaricato, mmHg)
// ==========================================
// La forza e' misurata come incremento di pressione rispetto al precarico
// (~40 mmHg), che viene tarato automaticamente mentre il sistema e' in IDLE.
#define HANDGRIP_ZERO_ADC       30.0f   // ADC a 0 mmHg (da tarare)
#define HANDGRIP_MAX_ADC        3800.0f // ADC a fondo scala (da tarare)
#define HANDGRIP_MAX_MMHG       300.0f  // pressione corrispondente a HANDGRIP_MAX_ADC
#define HG_SQUEEZE_START_DELTA  15.0f   // stretta: oltre precarico + 15 mmHg
#define HG_SQUEEZE_END_DELTA    7.0f    // fine stretta (isteresi)
#define HG_CALIB_SQUEEZES       3
#define HG_MIN_TARGET_DELTA     5.0f    // target minimo (evita trigger sul rumore)
#define HG_RELEASE_HOLD_MS      5000    // fine prova: sotto target per 5 s
#define HG_CALIB_TIMEOUT_MS     60000   // nessuna stretta per 60 s in taratura -> IDLE
#define HG_BASELINE_ALPHA       0.01f   // velocita' di inseguimento del precarico in IDLE
#define HANDGRIP_OUT2_FULL_MMHG 300.0f  // OUT2: 0..300 mmHg -> 0..5 V

// ==========================================
// 9. DEEP BREATHING
// ==========================================
#define BREATH_IN_TIME_MS       5000
#define BREATH_OUT_TIME_MS      5000
#define BREATH_CYCLE_MS         (BREATH_IN_TIME_MS + BREATH_OUT_TIME_MS)
#define DEEP_BREATHING_ACTS     10
#define DEEP_BREATHING_TOTAL_MS ((uint32_t)DEEP_BREATHING_ACTS * BREATH_CYCLE_MS) // 1m40s
#define DB_CLICK_LOCKOUT_MS     1500    // click ignorati subito dopo l'avvio

// ==========================================
// 10. COLD FACE / COLD PRESSOR
// ==========================================
#define COLD_FACE_DURATION_MS   60000

// ==========================================
// 11. PULSANTI
// ==========================================
#define BTN_DEBOUNCE_MS         30
#define BTN_DOUBLE_CLICK_MS     350     // finestra per il secondo click
#define BTN_CLICK_MAX_MS        1000    // pressioni piu' lunghe non sono click
#define BTN_LONG_PRESS_MS       5000    // pressione prolungata (Cold Face)

#endif
