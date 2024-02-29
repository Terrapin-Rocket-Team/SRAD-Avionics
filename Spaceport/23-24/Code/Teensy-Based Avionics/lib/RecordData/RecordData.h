#ifndef RECORD_DATA_H
#define RECORD_DATA_H

#include "psram.h"
// #include "../State/State.h"

extern int PRE_FLIGHT_DATA_DUMP_DURATION;
extern int PRE_FLIGHT_TIME_SINCE_LAST_DUMP;
extern int PRE_FLIGHT_TIME_OF_LAST_DUMP;

void recordData(char *data, int stage); //0 is preflight, 5 is postflight.
void dataToPSRAM(char *data);

#endif