#ifndef RECORD_DATA_H
#define RECORD_DATA_H

#include "psram.h"
// #include "../State/State.h"

extern int PRE_FLIGHT_DATA_DUMP_DURATION;
extern int PRE_FLIGHT_TIME_SINCE_LAST_DUMP;
extern int PRE_FLIGHT_TIME_OF_LAST_DUMP;

void recordData(String data, String stage);  //Stages can be "PreFlight", "Flight", "PostFlight"
void dataToPSRAM(String data);

#endif