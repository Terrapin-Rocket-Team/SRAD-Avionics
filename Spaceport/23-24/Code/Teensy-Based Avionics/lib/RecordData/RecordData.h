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

enum Mode
{
    FLIGHT,
    GROUND
};

struct BnoCalibData{
    float accelOffset[3];
    float accelRadius;
    float gyroOffset[3];
    float magOffset[3];
    float magRadius;
};

void recordFlightData(char *data); //0 is preflight, 5 is postflight.
void recordLogData(LogType type, const char *data, Dest dest = BOTH);
void recordLogData(double timeStamp, LogType type, const char *data, Dest dest = BOTH);
void setRecordMode(Mode mode);//Will enable or disable the PSRAM based on the mode. If mode is GROUND, PSRAM will be disabled and all data in PSRAM will be written to the SD card. If mode is FLIGHT, PSRAM will be enabled.
bool readCalibrationData(char *data, int len); //Reads the calibration data from the SD card and stores it in 'data'.
bool writeCalibrationData(char *data); //Writes the calibration data to the SD card.
#endif