#ifndef SD_CARD_H
#define SD_CARD_H

#include "SdFat.h"

#define COMPACT_WALKBACK_COUNT 50

extern SdFs sd;
extern FsFile logFile;
extern String logFileName;

bool setupSDCard(String csvHeader);  // Initializes the sensor
bool isSDReady();  // Returns whether the sensor is initialized


#endif