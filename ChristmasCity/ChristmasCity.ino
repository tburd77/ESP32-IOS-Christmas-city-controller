#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Ws2811.h"

#include <FastLED.h>
#include "BleManager.h"


void setup() {

  Serial.begin(115200);

  bleInit();

  setupCity();

}

void loop() {

  
    //if (testActive && (millis() - testStartMillis >= TEST_DURATION_MS)) {
    if (testActive) { 
      if (millis() - testStartMillis >= TEST_DURATION_MS) {
      testLED(99);
        //endTest();
      }
    }else {
        lightCity();
       
    }
 FastLED.show();
}
