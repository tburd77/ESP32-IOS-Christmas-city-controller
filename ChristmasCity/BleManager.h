

#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Ws2811.h"
#include "Storage.h"
//#include <NimBLEDevice.h>

extern bool testActive;
extern uint32_t testStartMillis;
extern const uint32_t TEST_DURATION_MS;

void bleInit();
void parseCommand(const String& cmd);
void sendLedsInfoChunked();

void startBinaryReception();
//void onBinaryComplete();
//void onBinaryComplete(const uint8_t* data, size_t len);
void verifyCRC();
void printArray();
void testLED(uint8_t led);
void endTest();