// Banco di prova per la verifica di equivalenza: esegue setup()/loop() del firmware
// con ingressi simulati e registra ogni effetto osservabile (DAC, LED, display,
// seriale, ritardi, configurazione dei pin) in una traccia con timestamp.
// Uscita: hash della traccia a ogni secondo simulato (checkpoint) e hash finale.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <random>
#include <functional>
#include <string>

void setup();
void loop();

static uint64_t g_now = 0;
static uint64_t g_hash = 1469598103934665603ULL;
static uint64_t g_events = 0;
static long g_winStart = -1, g_winEnd = -1;

void traceEvent(const std::string &s) {
    std::string line = std::to_string(g_now) + " " + s + "\n";
    for (unsigned char c : line) { g_hash ^= c; g_hash *= 1099511628211ULL; }
    g_events++;
    if ((long)g_now >= g_winStart && (long)g_now <= g_winEnd) fputs(line.c_str(), stdout);
}

// ---------------- Ingressi simulati ----------------
struct Key { double t; double v; };
static double interp(const std::vector<Key> &k, double t) {
    if (k.empty()) return 0;
    if (t <= k[0].t) return k[0].v;
    for (size_t i = 1; i < k.size(); i++)
        if (t <= k[i].t) {
            double a = (t - k[i-1].t) / (k[i].t - k[i-1].t);
            return k[i-1].v + a * (k[i].v - k[i-1].v);
        }
    return k.back().v;
}
struct Interval { uint64_t a, b; };
static bool inAny(const std::vector<Interval> &iv, uint64_t t) {
    for (auto &i : iv) if (t >= i.a && t < i.b) return true;
    return false;
}

struct Scenario {
    std::vector<Key> angle, valsalva, handgrip;   // gradi, mmHg, kg
    std::vector<Interval> btnMarker, btnT1, mpuFail;
    double noiseAdc = 6, noiseAngle = 0.15;
    uint64_t duration = 10000;
};
static Scenario S;

static uint64_t mix(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33; return x;
}
static uint64_t g_lastReadMs = ~0ULL; static uint32_t g_readIdx = 0;
static double noise(uint64_t salt) {
    if (g_lastReadMs != g_now) { g_lastReadMs = g_now; g_readIdx = 0; }
    uint64_t h = mix(g_now * 1000003ULL + salt * 7919ULL + (g_readIdx++) * 104729ULL);
    return ((h % 20001) / 10000.0) - 1.0; // -1..1
}

// ---------------- Stub Arduino ----------------
HardwareSerial Serial;
TwoWire Wire;

unsigned long millis() { return (unsigned long)g_now; }
void delay(uint32_t ms) { traceEvent("delay " + std::to_string(ms)); g_now += ms; }
void pinMode(uint8_t pin, uint8_t mode) { traceEvent("pinMode " + std::to_string(pin) + " " + std::to_string(mode)); }
void digitalWrite(uint8_t pin, uint8_t val) { traceEvent("digitalWrite " + std::to_string(pin) + " " + std::to_string(val)); }
int digitalRead(uint8_t pin) {
    if (pin == 35) return inAny(S.btnMarker, g_now) ? LOW : HIGH;
    if (pin == 32) return inAny(S.btnT1, g_now) ? LOW : HIGH;
    return HIGH;
}
uint16_t analogRead(uint8_t pin) {
    double adc = 0;
    if (pin == 12) adc = 30.0 + interp(S.valsalva, g_now) / 100.0 * 3770.0;
    else if (pin == 14) adc = 30.0 + (interp(S.handgrip, g_now) + 5.0) / 50.0 * 3770.0;
    adc += noise(pin) * S.noiseAdc;
    if (adc < 0) adc = 0;
    if (adc > 4095) adc = 4095;
    return (uint16_t)adc;
}
void analogReadResolution(uint8_t bits) { traceEvent("adcRes " + std::to_string(bits)); }
void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t a) { traceEvent("adcAtt " + std::to_string(pin) + " " + std::to_string((int)a)); }
void dacWrite(uint8_t pin, uint8_t v) { traceEvent("dac " + std::to_string(pin) + " " + std::to_string(v)); }
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    long d = in_max - in_min;
    if (d == 0) return out_min;
    return (x - in_min) * (out_max - out_min) / d + out_min;
}

void HardwareSerial::begin(unsigned long b) { traceEvent("serialBegin " + std::to_string(b)); }
size_t HardwareSerial::write(const char *s) { traceEvent(std::string("serial ") + s); return strlen(s); }
size_t HardwareSerial::print(long n, int base) { return write(std::to_string(n).c_str()); }
size_t HardwareSerial::print(unsigned long n, int base) { return write(std::to_string(n).c_str()); }
size_t HardwareSerial::print(double number, int digits) {
    // Stesso algoritmo di Print::printFloat di Arduino
    std::string out;
    if (std::isnan(number)) return write("nan");
    if (std::isinf(number)) return write("inf");
    if (number < 0.0) { out += '-'; number = -number; }
    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i) rounding /= 10.0;
    number += rounding;
    unsigned long int_part = (unsigned long)number;
    double remainder = number - (double)int_part;
    out += std::to_string(int_part);
    if (digits > 0) out += '.';
    while (digits-- > 0) { remainder *= 10.0; unsigned int d = (unsigned int)remainder; out += char('0' + d); remainder -= d; }
    return write(out.c_str());
}

// MPU-6050: 6 byte di accelerazione calcolati dall'angolo simulato
static uint8_t mpuBuf[6]; static int mpuAvail = 0, mpuPos = 0;
bool TwoWire::begin(int sda, int scl) { traceEvent("wireBegin " + std::to_string(sda) + " " + std::to_string(scl)); return true; }
void TwoWire::beginTransmission(int a) { traceEvent("wireBeginTx " + std::to_string(a)); }
size_t TwoWire::write(uint8_t b) { traceEvent("wireWrite " + std::to_string(b)); return 1; }
uint8_t TwoWire::endTransmission(bool stop) { traceEvent(std::string("wireEndTx ") + (stop ? "1" : "0")); return 0; }
uint8_t TwoWire::requestFrom(int a, int q, int stop) {
    traceEvent("wireReq " + std::to_string(a) + " " + std::to_string(q) + " " + std::to_string(stop));
    if (inAny(S.mpuFail, g_now)) { mpuAvail = 0; return 0; }
    double ang = (interp(S.angle, g_now) + noise(99) * S.noiseAngle) * M_PI / 180.0;
    int16_t ax = (int16_t)(noise(98) * 200), ay = (int16_t)(sin(ang) * 16384), az = (int16_t)(cos(ang) * 16384);
    int16_t v[3] = {ax, ay, az};
    for (int i = 0; i < 3; i++) { mpuBuf[2*i] = (uint8_t)((uint16_t)v[i] >> 8); mpuBuf[2*i+1] = (uint8_t)(v[i] & 0xff); }
    mpuAvail = 6; mpuPos = 0; return 6;
}
int TwoWire::available() { return mpuAvail - mpuPos; }
int TwoWire::read() { return mpuPos < mpuAvail ? mpuBuf[mpuPos++] : -1; }

void Adafruit_NeoPixel::show() {
    uint64_t h = 1469598103934665603ULL;
    for (int i = 0; i < n_; i++) { h ^= px_[i]; h *= 1099511628211ULL; }
    char b[96]; snprintf(b, sizeof b, "ledShow pin=%d n=%d br=%d %016llx", pin_, n_, bright_, (unsigned long long)h);
    traceEvent(b);
}
void TM1637Display::setBrightness(uint8_t b, bool on) { traceEvent("tmBright " + std::to_string(clk_) + " " + std::to_string(b) + " " + std::to_string(on)); }
void TM1637Display::showNumberDecEx(int num, uint8_t dots, bool lz, uint8_t len, uint8_t pos) {
    char b[96]; snprintf(b, sizeof b, "tmShow %d %d %d %d %d %d", clk_, num, dots, lz, len, pos); traceEvent(b);
}

// ---------------- Scenari ----------------
static void click(std::vector<Interval> &v, uint64_t t, uint64_t len = 120) { v.push_back({t, t + len}); }
static void dbl(std::vector<Interval> &v, uint64_t t) { click(v, t, 100); click(v, t + 200, 100); }
static void bounce(std::vector<Interval> &v, uint64_t t, uint64_t len) {
    for (int i = 0; i < 4; i++) v.push_back({t + i * 3, t + i * 3 + 1});
    v.push_back({t + 12, t + len});
    for (int i = 0; i < 3; i++) v.push_back({t + len + 2 + i * 3, t + len + 3 + i * 3});
}

static void buildScenario(const std::string &name) {
    Scenario s;
    if (name == "idle") { s.duration = 15000; }
    else if (name == "tilt") {
        click(s.btnT1, 1000);
        s.angle = {{0,0},{3000,0},{26000,70},{50000,70},{75000,0},{90000,0}};
        click(s.btnT1, 85000);
        s.duration = 95000;
    } else if (name == "tilt_slow_noisy") {
        s.noiseAngle = 0.8;
        s.angle = {{0,0},{2000,0},{12000,20},{20000,20},{50000,62},{52000,58},{80000,58},{83000,50},{86000,50},{120000,0},{140000,0}};
        s.duration = 150000;
    } else if (name == "tilt_stuck_valsalva20") {
        s.angle = {{0,0},{2000,0},{7000,20},{60000,20}};
        s.valsalva = {{0,0},{15000,0},{17000,45},{30000,45},{31000,0},{60000,0}};
        click(s.btnMarker, 40000);
        s.duration = 60000;
    } else if (name == "tilt_static35") {
        s.angle = {{0,0},{2000,0},{2500,35},{20000,35},{21000,0},{40000,0}};
        s.duration = 40000;
    } else if (name == "overrange") {
        s.valsalva = {{0,0},{2000,0},{4000,130},{10000,130},{11000,0},{20000,0}};
        s.handgrip = {{0,0},{25000,0},{26000,80},{27000,0},{40000,0}};
        s.duration = 40000;
    } else if (name == "cold_face") {
        s.btnMarker.push_back({2000, 7500});    // pressione lunga in IDLE: avvio
        click(s.btnMarker, 20000);              // click singolo: fine
        s.valsalva = {{0,0},{30000,0},{31000,40},{42000,40},{43000,0},{60000,0}};
        s.btnMarker.push_back({34000, 40000});  // pressione lunga durante la Valsalva
        s.duration = 60000;
    } else if (name == "marker_queue") {
        click(s.btnMarker, 1000); click(s.btnMarker, 1500); click(s.btnMarker, 2000);
        s.duration = 8000;
    } else if (name == "valsalva") {
        click(s.btnT1, 500);
        s.valsalva = {{0,0},{3000,0},{5000,45},{17000,42},{18000,5},{30000,5},
                      {35000,0},{36000,22},{45000,22},{46000,30},{50000,50},{52000,8},{54000,8},{55000,30},{60000,30},{61000,0},{75000,0}};
        dbl(s.btnMarker, 47000);
        click(s.btnMarker, 49000);
        s.duration = 80000;
    } else if (name == "handgrip") {
        click(s.btnT1, 500);
        std::vector<Key> k = {{0,0},{3000,0}};
        double t = 3000;
        for (int i = 0; i < 3; i++) { k.push_back({t+800,18.0+i*3}); k.push_back({t+1600,0}); k.push_back({t+3000,0}); t += 3000; }
        k.insert(k.end(), {{t+8000,0},{t+9000,12},{t+19000,12},{t+19500,3},{t+22000,3},{t+22500,12},{t+40000,12},{t+41000,0},{t+60000,0}});
        s.handgrip = k;
        s.duration = (uint64_t)t + 60000;
    } else if (name == "handgrip_partial") {
        s.handgrip = {{0,0},{2000,0},{2500,10},{3000,10},{3500,0},{20000,0},{20500,4.9},{21000,5.1},{21500,4.8},{22000,0},{40000,0}};
        click(s.btnMarker, 30000);
        s.duration = 40000;
    } else if (name == "deep_breathing") {
        dbl(s.btnMarker, 1000);
        click(s.btnMarker, 7000);
        click(s.btnMarker, 20000);             // click durante la registrazione
        dbl(s.btnMarker, 110000);
        dbl(s.btnMarker, 125000);
        click(s.btnMarker, 129000, 80);
        dbl(s.btnMarker, 133000);
        dbl(s.btnMarker, 140000);
        s.duration = 150000;
    } else if (name == "buttons") {
        click(s.btnMarker, 1000);
        click(s.btnMarker, 1400, 40);          // troppo breve
        s.btnMarker.push_back({3000, 9000});   // pressione lunga 6 s
        click(s.btnMarker, 9300);              // marker mentre il precedente e' attivo
        s.btnMarker.push_back({12000, 14500}); // pressione media 2,5 s
        bounce(s.btnMarker, 17000, 150);
        click(s.btnMarker, 20000); click(s.btnMarker, 20250); click(s.btnMarker, 20500); // triplo click
        bounce(s.btnT1, 2000, 200); bounce(s.btnT1, 6000, 200);
        click(s.btnT1, 23000, 30); click(s.btnT1, 23040, 30);
        s.duration = 30000;
    } else if (name.rfind("fuzz", 0) == 0) {
        unsigned seed = (unsigned)std::stoul(name.substr(4));
        std::mt19937 rng(seed);
        auto U = [&](double a, double b) { return std::uniform_real_distribution<double>(a, b)(rng); };
        auto I = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };
        s.duration = 20 * 60 * 1000;
        s.noiseAngle = U(0.05, 1.0); s.noiseAdc = U(2, 30);
        auto walk = [&](std::vector<Key> &k, double maxv, double pZero) {
            double t = 0; k.push_back({0, 0});
            while (t < s.duration) {
                t += U(300, 25000);
                double v = (U(0, 1) < pZero) ? 0 : U(0, maxv);
                if (U(0, 1) < 0.3) k.push_back({t, k.back().v});
                k.push_back({t + U(50, 8000), v});
                t = k.back().t;
            }
        };
        walk(s.angle, 85, 0.5); walk(s.valsalva, 70, 0.6); walk(s.handgrip, 30, 0.6);
        uint64_t t = 0;
        while (t < s.duration) {
            t += I(200, 15000);
            int g = I(0, 9);
            if (g <= 2) click(s.btnMarker, t, I(55, 400));
            else if (g == 3) dbl(s.btnMarker, t);
            else if (g == 4) s.btnMarker.push_back({t, t + (uint64_t)I(4800, 7000)});
            else if (g == 5) bounce(s.btnMarker, t, I(60, 300));
            else if (g == 6) s.btnMarker.push_back({t, t + (uint64_t)I(400, 4000)});
            else if (g == 7) click(s.btnT1, t, I(30, 300));
            else if (g == 8) bounce(s.btnT1, t, I(60, 300));
            else s.mpuFail.push_back({t, t + (uint64_t)I(5, 3000)});
            t = std::max(t, s.btnMarker.empty() ? t : s.btnMarker.back().b);
        }
    } else {
        fprintf(stderr, "scenario sconosciuto: %s\n", name.c_str()); exit(2);
    }
    S = s;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "uso: sim <scenario> [finestra_da_ms finestra_a_ms]\n"); return 2; }
    buildScenario(argv[1]);
    if (argc >= 4) { g_winStart = atol(argv[2]); g_winEnd = atol(argv[3]); }
    setup();
    uint64_t nextCheck = 1000;
    while (g_now < S.duration) {
        loop();
        g_now += 1;
        while (g_now >= nextCheck) {
            if (g_winStart < 0) printf("checkpoint %llu %016llx\n", (unsigned long long)nextCheck, (unsigned long long)g_hash);
            nextCheck += 1000;
        }
    }
    if (g_winStart < 0) printf("final %016llx events=%llu\n", (unsigned long long)g_hash, (unsigned long long)g_events);
    return 0;
}
