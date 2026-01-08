#include "Storage.h"

Preferences prefs;
//StoredData defaultsA;
//LedsInfo defaultsA;

void saveLedsInfoArray(const LedsInfo* arr, size_t count) {
  prefs.begin("leds", false);  // RW
  prefs.putUInt("count", count);   // optional safety
  prefs.putBytes("array", arr, sizeof(LedsInfo) * count);
  prefs.end();
}

bool loadLedsInfoArray(LedsInfo* arr, size_t maxCount, size_t& outCount) {
  prefs.begin("leds", true);   // read-only

  outCount = prefs.getUInt("count", 0);
    Serial.print("outCount = ");Serial.println(outCount);
  if (outCount == 0 || outCount > maxCount) {
    prefs.end();
    return false;
  }

  size_t len = prefs.getBytes("array", arr, sizeof(LedsInfo) * outCount);
  prefs.end();

  return (len == sizeof(LedsInfo) * outCount);
}

void loadDefault_LedsInfo() {

    for (int i = 0; i < NUM_LEDS; i++) {
      //leds_Info[i].interval = random(80, 200);
      //leds_Info[i].lastUpdate = millis();
   
      if (i % 3 == 0) {
        leds_Info[i].house = 1;
        leds_Info[i].baseBrightness = random(100, 180);

        leds_Info[i].candleRed = random8(200, 255);//random(1000);
      leds_Info[i].candleGreen = random8(80, 140);//random(1000);
      leds_Info[i].candleBlue = 0;
       
      }else {
        leds_Info[i].house = 0;
         leds_Info[i].baseBrightness = 10;
      
      leds_Info[i].candleRed = 10;//random(1000);
      leds_Info[i].candleGreen = 10;//random(1000);
      leds_Info[i].candleBlue = 10;//random(1000);
      }
    }

    saveLedsInfoArray(leds_Info, NUM_LEDS);
}
