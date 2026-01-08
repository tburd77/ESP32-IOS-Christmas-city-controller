#pragma once
#include <FastLED.h>


////#define LED_PIN 5
#define NUM_LEDS 50
#define BRIGHTNESS 150

#pragma pack(push, 1)
struct LedsInfo {
  uint32_t interval;
  uint32_t candleBlue;
  uint8_t  baseBrightness;
  uint8_t  house;
  int32_t  candleRed;
  int32_t  candleGreen;
};
#pragma pack(pop)

extern LedsInfo leds_Info[NUM_LEDS];
extern CRGB leds[NUM_LEDS];

// --------------------

void initLedTimers();
void updateLeds();
void setupCity();
void lightCity();
void updateCandles(uint8_t i);
void setInitialColors();
CRGB candleColor(uint8_t brightness);
void randomFlicker(uint8_t i);
