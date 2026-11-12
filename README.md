# ANSCOVERY - Scheda di controllo ESP32 (SparkBio)

Firmware della scheda di controllo (SE) del kit ANSCOVERY per la valutazione del
Sistema Nervoso Autonomo (batteria di Ewing). Uso esclusivo di ricerca (RUO).

La SE legge i sensori delle manovre, guida paziente e operatore con la barra LED
e i due display dei tempi, e marca le manovre sul poligrafo (Finapres NOVA /
Task Force Monitor) con due uscite analogiche 0-5 V.

## Hardware

| Funzione | Componente | GPIO |
|---|---|---|
| Tempo 1 / Tempo 2 | 2 display TM1637 (MM:SS) | CLK/DIO 2/4 e 5/18 |
| Barra LED | WS2812B, 6 barre da 8 LED | 15 |
| Pulsante MARKER / AZZERA T1 | attivi bassi | 35 / 32 |
| Pressione Valsalva (SP1) | ADC | 12 |
| Pressione manicotto Hand Grip | ADC | 14 |
| Angolo lettino | MPU-6050 (I2C 0x68) | SDA 21 / SCL 22 |
| OUT1 marker / OUT2 continuo | DAC 8 bit | 25 / 26 |

Librerie Arduino: `Adafruit NeoPixel`, `TM1637Display` (Avishay Orpaz).

## Manovre

| Manovra | Avvio | OUT1 (inizio / fine) | OUT2 | Fine |
|---|---|---|---|---|
| Tilt Test | angolo > 30 gradi e in salita | 1 V: 1 s salita, 2 s picco, 1 s discesa, 2 s fine | angolo (0-90 gradi) | angolo stabile sotto 30 gradi |
| Valsalva | SP1 > 25 mmHg | 2 V: 1 s / 2 s | pressione SP1 | SP1 < 10 mmHg per 3 s |
| Hand Grip | stretta oltre precarico + 15 mmHg | 3 V: 1 s / 2 s | pressione manicotto | sotto target per 5 s |
| Deep Breathing | doppio click; click singolo avvia la registrazione | 4 V: 1 s / 2 s | sinusoide del ritmo | 10 atti (100 s) |
| Cold Face | pressione MARKER >= 5 s | 5 V: 1 s / 2 s | 0 V | dopo 60 s |
| Marker generico | click singolo | 1.5 V: 1 s | - | - |

- **AZZERA T1**: ogni pressione azzera e fa ripartire Tempo 1.
- **Doppio click durante un test**: interrompe il test e torna in IDLE (senza marker).
- **Hand Grip**: la stretta che avvia il test e' la prima delle 3 di taratura;
  target = 1/3 della media dei picchi (sopra il precarico, tarato automaticamente in IDLE).
- **Tempo 2** a fine manovra resta fermo sulla durata della fase (Cold Face: azzerato).
- I marker vengono accodati: due eventi ravvicinati producono due impulsi separati
  da 300 ms a 0 V.

## Uscite su seriale (in attesa dello stadio 0-5 V)

Tutte le tensioni nel firmware sono "logiche" 0-5 V: 5 V corrisponde al valore DAC 255.
Finche' non c'e' lo stadio op-amp (guadagno ~1.515) la tensione reale sui pin 25/26
e' il valore logico diviso 1.515; i valori corretti si leggono sulla seriale (115200 baud):

- `SERIAL_PLOTTER_MODE 0` (default, **Serial Monitor**): eventi (`[OUT1] 2.00 V per 1000 ms`)
  e una riga di stato al secondo con T1, T2, OUT1, OUT2 e sensori.
- `SERIAL_PLOTTER_MODE 1` (**Serial Plotter**): solo `OUT1:x,OUT2:y` ogni 50 ms,
  per vedere i due canali come li vedrebbe il poligrafo.

## Taratura

In `Config.h` vanno tarati sul sensore reale `VALSALVA_ZERO_ADC` / `VALSALVA_MAX_ADC`
e `HANDGRIP_ZERO_ADC` / `HANDGRIP_MAX_ADC` / `HANDGRIP_MAX_MMHG`. In IDLE la riga di
stato ogni 5 s mostra angolo e pressioni lette.

## Note hardware

- **GPIO12 (sensore Valsalva) e' un pin di strapping**: all'accensione l'ESP32 lo legge
  per scegliere la tensione della flash. Se in quel momento il sensore porta il pin
  a livello alto, la scheda non si avvia (reset continui). Se succede: spostare il
  sensore su un pin ADC1 libero (36, 39 o 33) oppure fissare la tensione della flash
  a 3.3 V via eFuse (`espefuse.py set_flash_voltage 3.3V`, operazione irreversibile).
- GPIO12/14 sono sull'ADC2, non utilizzabile se in futuro si attivano WiFi/Bluetooth.
- Il DAC dell'ESP32 arriva a circa 3.2 V: lo stadio op-amp e' necessario per lo 0-5 V.

## Simulatore su PC

`tools/sim/build.sh <scenario>` compila il firmware con stub minimali di Arduino ed
esegue uno scenario (`tilt`, `valsalva20`, `handgrip`, `db`, `cold`, `t1`), stampando
eventi e marker con il tempo simulato. Serve a verificare la logica senza la scheda;
non sostituisce la prova sull'hardware.
