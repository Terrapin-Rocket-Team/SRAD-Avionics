#ifndef RECORD_DATA_H
#define RECORD_DATA_H

#include "psram.h"
#include "sdCard.h"

extern PSRAM *ram;


enum LogType
{
    LOG,
    ERROR,
    WARNING,
    INFO
};
enum Dest
{
    BOTH,
    TO_USB,
    TO_FILE
};

void recordFlightData(char *data); //0 is preflight, 5 is postflight.
void recordLogData(LogType type, const char *data, Dest dest = BOTH);
void dataStageUpdate(int stage);//kind of bad...
#endif