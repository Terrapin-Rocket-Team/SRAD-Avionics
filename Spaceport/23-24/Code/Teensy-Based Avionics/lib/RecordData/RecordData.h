#ifndef RECORD_DATA_H
#define RECORD_DATA_H

#include "psram.h"
// #include "../State/State.h"

extern int PRE_FLIGHT_DATA_DUMP_DURATION;
extern int PRE_FLIGHT_TIME_SINCE_LAST_DUMP;
extern int PRE_FLIGHT_TIME_OF_LAST_DUMP;

void recordData(char *data, char *stage); // Stages can be "Pre-Flight", "Stage X", "Post-Flight"
void dataToPSRAM(char *data);

#endif