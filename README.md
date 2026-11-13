# ANSCOVERY - Scheda di controllo ESP32 (SparkBio)

Firmware della scheda di controllo del kit ANSCOVERY per i test del sistema
nervoso autonomo (batteria di Ewing). Uso esclusivo di ricerca (RUO).

La scheda riconosce da sola la manovra in corso dai sensori e dal pulsante
MARKER, guida il paziente con la barra LED, mostra Tempo 1 e Tempo 2 sui
display e marca gli eventi sul poligrafo con due uscite analogiche
(OUT1 marker, OUT2 segnale continuo).

## Hardware

| Funzione | Componente | GPIO |
|---|---|---|
| Tempo 1 / Tempo 2 | 2 display TM1637 (MM:SS) | CLK/DIO 2/4 e 5/18 |
| Barra LED | WS2812B, 48 LED (6 barre da 8) | 15 |
| Pulsante MARKER / AZZERA T1 | attivi bassi | 35 / 32 |
| Pressione Valsalva | ADC | 12 |
| Forza Hand Grip | ADC | 14 |
| Angolo lettino | MPU-6050 (I2C 0x68) | SDA 21 / SCL 22 |
| OUT1 marker / OUT2 continuo | DAC 8 bit (0-3,3 V) | 25 / 26 |

## Struttura del codice

| File | Contenuto |
|---|---|
| `ANSCOVERY_ESP32.ino` | `setup()` e `loop()` |
| `Config.h` | tutti i parametri: pin, soglie, tempi, tensioni dei marker, tarature |
| `FSM.h/.cpp` | riconoscimento della manovra in IDLE, pulsante MARKER, barra LED, stato su seriale |
| `Maneuvers.h` | interfaccia delle quattro manovre |
| `TiltTest.cpp`, `Valsalva.cpp`, `Handgrip.cpp`, `DeepBreathing.cpp`, `ColdFace.cpp` | una manovra per file, ciascuna con la sua macchina a stati |
| `Sensors.h/.cpp` | angolo lettino, pressione Valsalva, forza Hand Grip |
| `Buttons.h/.cpp` | click singolo, doppio click, pressione lunga, AZZERA T1 |
| `Outputs.h/.cpp` | marker su OUT1 (in coda), segnale su OUT2, stampa delle uscite |
| `Timers.h/.cpp` | Tempo 1 e Tempo 2 |
| `Leds.h/.cpp`, `Display.h/.cpp` | barra LED e display dei tempi |

## Uscite analogiche (DAC 0-3,3 V, senza stadio 0-5 V)

Entrambe a 0 V all'accensione.

**OUT1 - marker** (impulsi da 1 s = inizio, 2 s = fine; i marker ravvicinati vanno in coda,
separati da 300 ms a 0 V): Tilt 1 V, click singolo 1,5 V, Valsalva 2 V, Hand Grip 3 V,
Deep Breathing 4 V, Cold Face 5 V. Oltre 3,3 V il valore e' limitato al massimo del DAC.

**OUT2 - segnale continuo**: Tilt = angolo (0-90 gradi), Valsalva = pressione (0-100 mmHg),
Hand Grip = forza (0-50 kg), Deep Breathing = sinusoide del ritmo respiratorio (0 a inizio
inspirazione, massimo a fine inspirazione, periodo 10 s), IDLE e Cold Face = 0 V.

**Cold Face**: pressione del MARKER per almeno 5 s in IDLE -> marker 5 V / 1 s; click singolo
-> fine test, marker 5 V / 2 s. Durante le altre manovre la pressione lunga da' solo il
marker 5 V / 1 s.

## Compilazione

1. Arduino IDE 2.x con il pacchetto schede **esp32** di Espressif, scheda **ESP32 Dev Module**.
2. Librerie: **Adafruit NeoPixel**, **TM1637Display** (Avishay Orpaz).
3. Aprire `ANSCOVERY_ESP32/ANSCOVERY_ESP32.ino` e caricare. Serial Monitor a 115200 baud.

## Verifica di equivalenza

`tools/equivalenza/confronta.sh <commit>` esegue sul PC la versione attuale e quella
del commit indicato con sensori e pulsanti simulati (19 scenari, 8 dei quali casuali
da 20 minuti) e confronta istante per istante tutto cio' che il firmware produce:
uscite DAC, barra LED, display, seriale, ritardi. Serve a verificare che una modifica
di sola forma non cambi il comportamento. Richiede `g++`.
