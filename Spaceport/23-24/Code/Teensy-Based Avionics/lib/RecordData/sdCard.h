#ifndef SD_CARD_H
#define SD_CARD_H

#include "SdFat.h"


extern SdFs sd;
extern FsFile logFile;
extern FsFile flightDataFile;
extern char logFileName[24];
extern char flightDataFileName[24];

bool setupSDCard(); // Initializes the sensor
bool isSDReady();  // Returns whether the sensor is initialized
void sendSDCardHeader(const char *csvHeader); // Sends the header to the SD card

#endif