#ifndef SD_CARD_H
#define SD_CARD_H

#include "SdFat.h"


extern SdFs sd;
extern FsFile logFile;
extern FsFile flightDataFile;
extern FsFile calibFile;
extern char logFileName[];
extern char flightDataFileName[];
extern char calibFileName[];

bool setupSDCard(); // Initializes the card
bool isSDReady();  // Returns whether the card is initialized
void sendSDCardHeader(const char *csvHeader); // Sends the header to the SD card for the csv file.

#endif