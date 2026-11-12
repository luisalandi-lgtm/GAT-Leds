// Simulatore su PC: sostituisce sensori, pulsanti e DAC con valori generati
// dagli scenari qui sotto e stampa i messaggi seriali con il tempo simulato.
#include <Arduino.h>
#include <Wire.h>
#include <functional>
#include <string>
uint32_t g_now=0; SerialC Serial; TwoWire Wire;
float simAngle=0, simVals=0, simHg=40; int btnMarker=1, btnT1=1;
int dac1=0,dac2=0;
void SerialC::printf_t(const char*f,va_list a){ ::printf("%7.2fs ",g_now/1000.0); ::vprintf(f,a);} 
void pinMode(int,int){} void analogReadResolution(int){} void analogSetPinAttenuation(int,int){}
int digitalRead(int p){ return p==35?btnMarker:btnT1; }
int analogRead(int p){ if(p==12) return (int)(30+simVals/100*3770); return (int)(30+simHg/300*3770); }
void dacWrite(int p,int v){ if(p==25){ dac1=v; } else dac2=v; }
static int rd=0; static int16_t vals[3];
int TwoWire::endTransmission(bool){ return 0; }
int TwoWire::requestFrom(uint8_t,uint8_t){ float r=simAngle*M_PI/180; vals[0]=0; vals[1]=(int16_t)(sin(r)*16384); vals[2]=(int16_t)(cos(r)*16384); rd=0; return 6; }
int TwoWire::read(){ int16_t v=vals[rd/2]; int b = (rd%2==0)? (v>>8)&0xff : v&0xff; rd++; return b; }
void setup(); void loop();
void run(uint32_t ms, std::function<void(uint32_t)> f){ uint32_t end=g_now+ms; uint32_t t0=g_now; while(g_now<end){ f(g_now-t0); loop(); g_now+=5; } }
int main(int argc,char**argv){
  std::string sc = argc>1?argv[1]:"all";
  setup();
  run(2000,[](uint32_t){});
  if(sc=="tilt"){ // salita 0->70 a 3 deg/s, plateau, discesa
    run(25000,[](uint32_t t){ simAngle = fmin(70, t/1000.0*3); });
    run(20000,[](uint32_t){});
    run(30000,[](uint32_t t){ simAngle = fmax(0, 70 - t/1000.0*3); });
    run(5000,[](uint32_t){});
  }
  if(sc=="valsalva20"){ // lettino statico a 20 gradi + valsalva
    run(10000,[](uint32_t t){ simAngle = fmin(20, t/1000.0*3); });
    run(15000,[](uint32_t t){ simVals = t<15000 ? 42 : 0; });
    run(6000,[](uint32_t){ simVals=0; });
  }
  if(sc=="handgrip"){
    for(int i=0;i<3;i++){ run(2000,[](uint32_t t){ simHg = 40 + 60*sin(M_PI*t/2000.0); }); run(2000,[](uint32_t){ simHg=40;}); }
    run(8000,[](uint32_t){ simHg=40; });
    run(30000,[](uint32_t){ simHg=40+25; });
    run(8000,[](uint32_t){ simHg=40; });
  }
  if(sc=="db"){
    run(100,[](uint32_t){btnMarker=0;}); run(100,[](uint32_t){btnMarker=1;}); run(100,[](uint32_t){btnMarker=0;}); run(100,[](uint32_t){btnMarker=1;});
    run(4000,[](uint32_t){});
    run(150,[](uint32_t t){btnMarker = (t<12)?((t/3)%2):(t<120?0:1);} ); // click con rimbalzi
    run(103000,[](uint32_t t){ btnMarker=1; if(t%10000==2500) printf("   OUT2=%.2fV dac2=%d\n", dac2*5.0/255, dac2); });
  }
  if(sc=="cold"){
    run(5200,[](uint32_t){btnMarker=0;}); run(100,[](uint32_t){btnMarker=1;});
    run(63000,[](uint32_t){});
  }
  if(sc=="t1"){ run(100,[](uint32_t t){btnT1 = (t<10)?((t/3)%2):0;}); run(100,[](uint32_t){btnT1=1;}); }
}
