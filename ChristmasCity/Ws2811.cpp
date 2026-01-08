
#include "Ws2811.h"

//#define NUM_LEDS 50
#define DATA_PIN 5
#define LED_TYPE WS2811

CRGB leds[NUM_LEDS];

struct LedTimer {
  uint32_t on_Inteval;     // ms
  uint32_t on_Update;   // millis()
  uint32_t off_Inteval;     // ms
  uint32_t off_Update;   // millis()
  uint32_t lites_On;

  uint32_t interval;
  uint32_t lastUpdate;
};


LedTimer ledTimers[NUM_LEDS];
LedsInfo leds_Info[NUM_LEDS];



void initLedTimers() {
  uint32_t now = millis();

  for (int i = 0; i < NUM_LEDS; i++) {
    ledTimers[i].on_Update = now;
    ledTimers[i].on_Inteval = random(10000, 20000);;// random(60000, 100000);
    ledTimers[i].off_Update = now;
    ledTimers[i].off_Inteval = random(1000, 3000);;// random(60000, 100000);
  }
}

void initCandles(uint8_t candle) {
  uint32_t now = millis();

   // leds_Info[candle].lastUpdate = now;
ledTimers[candle].lastUpdate = now;
 ledTimers[candle].interval = random(20, 120);

   // leds_Info[candle].interval = random(20, 120);     // ms (fast → slow)
    leds_Info[candle].baseBrightness = random(110, 160);
}

void setInitialColors() {

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < NUM_LEDS; i++) {
    if (leds_Info[i].house == 1) {
      initCandles(i);
      ledTimers[i].lites_On = 1;
      randomFlicker(i);
    }else {
      leds_Info[i].baseBrightness = 10;
      leds[i] = CRGB(10, 10, 10);
    
    }
  }

}

void setupCity() {
  delay(500);
 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  randomSeed(esp_random());  // ESP32 true RNG
  initLedTimers();
  setInitialColors();

}

void updateCandles(uint8_t i) {
  uint32_t now = millis();

   // if (now - leds_Info[i].lastUpdate >= leds_Info[i].interval) {
  if (now - ledTimers[i].lastUpdate >= ledTimers[i].interval) {
      ledTimers[i].lastUpdate = now;
      //leds_Info[i].lastUpdate = now;
      randomFlicker(i);
    }
}

void randomFlicker(uint8_t i) {
    uint8_t flicker = random8(
        leds_Info[i].baseBrightness - 20,
        leds_Info[i].baseBrightness + 10
      );

      leds[i] = candleColor(flicker);
      leds_Info[i].candleRed = leds[i].r;
      leds_Info[i].candleGreen = leds[i].g;
}

CRGB candleColor(uint8_t brightness) {
  
  return CRGB(
    random8(200, 255),        // red dominant
    random8(80, 140),         // green lower
    0                         // NO BLUE
  ).nscale8_video(brightness);
}

void lightCity() {
  uint32_t now = millis();


  for (int i = 0; i < NUM_LEDS; i++) {
     if (leds_Info[i].house == 1) {
      //if (ledTimers[i].house == 1) {
        if (ledTimers[i].lites_On == 1) {
            updateCandles(i);
          if (now - ledTimers[i].on_Update >= ledTimers[i].on_Inteval) {
            ledTimers[i].on_Update = now;
            ledTimers[i].on_Inteval = random(10000, 20000);
           
            int a = random(0, 4);
            if (a == 3) {
              ledTimers[i].lites_On = 0;
               leds[i] = CRGB(0, 0, 0);
               ledTimers[i].off_Inteval = random(10000, 40000);
               ledTimers[i].off_Update = now;
             //  Serial.print("on lights OFF ");
            }else {
              ledTimers[i].lites_On = 1;
             // Serial.print("on lights ON ");
            }
          }
        }else {
          if (now - ledTimers[i].off_Update >= ledTimers[i].off_Inteval) {
            ledTimers[i].off_Update = now;
            ledTimers[i].off_Inteval = random(10000, 40000);
            int a = random(0, 12);
            if (a == 9) {
                ledTimers[i].lites_On = 0;
                leds[i] = CRGB(0, 0, 0);
              //  Serial.print("off lights OFF ");
              }else {
                ledTimers[i].lites_On = 1;
                ledTimers[i].on_Inteval = random(10000, 20000);
                ledTimers[i].on_Update = now;
               // Serial.print("off lights ON ");
              }
          }
        }
      //  Serial.println(i);

    }else {
      
      leds[i] = CRGB(10, 10, 10);
      //leds[i] = CRGB(255, 255, 255);
      leds_Info[i].baseBrightness = 1;
    }
  }
//  Serial.println("*****************");
}//

