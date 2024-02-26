#ifndef LOG_H
#define LOG_H

#include <Arduino.h> //for Serial output
#include "RecordData.h"

enum LogType
{
    LOG,
    ERROR,
    WARNING,
    INFO
};

void logLine(LogType type, char *data);//expects a c-string
void dumpLog();//dumps to SD card. May take a lot of time to complete, so be careful with this.

#endif