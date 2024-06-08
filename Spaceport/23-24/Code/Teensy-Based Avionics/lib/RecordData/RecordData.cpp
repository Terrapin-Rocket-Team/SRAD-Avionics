#include "RecordData.h"

static const char *logTypeStrings[] = {"LOG", "ERROR", "WARNING", "INFO"};
static Mode mode = GROUND;
void recordFlightData(char *data)
{
    if (!isSDReady())
        return;
    if (mode == GROUND || !ram->isReady())
    {
        flightDataFile = sd.open(flightDataFileName, FILE_WRITE); // during preflight, print to SD card constantly. ignore PSRAM for this stage.
        if (flightDataFile)
        {
            flightDataFile.println(data);
            flightDataFile.close();
        }
    }
    else
        ram->println(data); // while in flight, print to PSRAM for later dumping to SD card.
}

void recordLogData(LogType type, const char *data, Dest dest)
{
    recordLogData(millis() / 1000.0, type, data, dest);
}

void recordLogData(double timeStamp, LogType type, const char *data, Dest dest)
{
    int size = 15 + 7; // 15 for the timestamp and extra chars, 7 for the log type
    char logPrefix[size];
    snprintf(logPrefix, size, "%.3f - [%s] ", timeStamp, logTypeStrings[type]);

    if (dest == BOTH || dest == TO_USB)
    {
        Serial.print(logPrefix);
        Serial.println(data);
    }
    if ((dest == BOTH || dest == TO_FILE) && isSDReady())
    {
        if (mode == GROUND || !ram->isReady())
        {
            logFile = sd.open(logFileName, FILE_WRITE); // during preflight, print to SD card constantly. ignore PSRAM for this stage. With both files printing, may be bad...
            if (logFile)
            {
                logFile.print(logPrefix);
                logFile.println(data);
                logFile.close();
            }
        }
        else // while in flight, print to PSRAM for later dumping to SD card.
        {
            ram->print(logPrefix, false);
            ram->println(data, false);
        }
    }
}

void setRecordMode(Mode m)
{
    if (mode == FLIGHT && m == GROUND && ram->isReady())
    {
        ram->dumpFlightData();
        ram->dumpLogData();
        bb.aonoff(LED_BUILTIN, 2000); // turn on LED for 2 seconds to indicate that the PSRAM is being dumped to the SD card.
    }
    mode = m;
}

bool readCalibrationData(char *data, int len)
{
    if (isSDReady())
    {
        calibFile = sd.open(calibFileName, FILE_READ);
        if (calibFile)
        {
            int i = calibFile.read(data, len);
            if(i > 0)
                data[i] = '\0';
            calibFile.close();
            return i > 0; // most basic check to see if the file was read.
        }
    }
    return false;
}

bool writeCalibrationData(char *data)
{
    if (isSDReady())
    {
        sd.remove(calibFileName);
        calibFile = sd.open(calibFileName, FILE_WRITE);
        if (calibFile)
        {
            calibFile.print(data);
            calibFile.close();
            return true;
        }
    }
    return false;
}