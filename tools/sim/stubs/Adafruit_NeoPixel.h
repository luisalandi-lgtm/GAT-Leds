#pragma once
#include <Arduino.h>
#define NEO_GRB 0
#define NEO_KHZ800 0
struct Adafruit_NeoPixel{ uint8_t px[48*3]; int n; Adafruit_NeoPixel(int n_,int,int):n(n_){memset(px,0,sizeof px);} void begin(){} void setBrightness(int){} void clear(){memset(px,0,sizeof px);} void show(){} void fill(uint32_t c){for(int i=0;i<n;i++)setPixelColor(i,c);} static uint32_t Color(uint8_t r,uint8_t g,uint8_t b){return (r<<16)|(g<<8)|b;} void setPixelColor(int i,uint32_t c){px[i*3]=c>>16;px[i*3+1]=c>>8;px[i*3+2]=c;} uint8_t* getPixels(){return px;} int numPixels(){return n;} };
