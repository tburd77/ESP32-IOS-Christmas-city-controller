#pragma once

#include <Preferences.h>
//#include "StoredData.h"
#include "Ws2811.h"


void saveLedsInfoArray(const LedsInfo* arr, size_t count);
bool loadLedsInfoArray(LedsInfo* arr, size_t maxCount, size_t& outCount);
void loadDefault_LedsInfo() ;

//void saveStoredData(const LedsInfo& a);
//bool loadStoredData(LedsInfo& a);
//void loadDefault_LedsInfo();
/*void saveStoredData(const StoredData& a);
bool loadStoredData(StoredData& a);*/
