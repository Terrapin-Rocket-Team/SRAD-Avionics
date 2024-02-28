#include "RecordData.h"

static const char *logTypeStrings[] = {"LOG", "ERROR", "WARNING", "INFO"};
static int dataStage = 0;
void recordFlightData(char *data)
{
    if ((dataStage == 0 || dataStage > 4) && isSDReady())
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
    int size = 15 + 7; // 15 for the timestamp and extra chars, 7 for the log type
    char logPrefix[size];
    snprintf(logPrefix, size, "%.3f - [%s] ", millis() / 1000.0, logTypeStrings[type]);

    if (dest == BOTH || dest == TO_USB)
    {
        if (!Serial)
            Serial.begin(9600);

        Serial.print(logPrefix);
        Serial.println(data);
    }
    if (dest == BOTH || dest == TO_FILE)
    {
        if ((dataStage == 0 || dataStage > 4) && isSDReady())
        {
            logFile = sd.open(logFileName, FILE_WRITE); // during preflight, print to SD card constantly. ignore PSRAM for this stage. With both files printing, may be bad...
            if (logFile)
            {
                logFile.print(logPrefix);
                logFile.println(data);
                logFile.close();
            }
        }
        else if (ram->isReady()) // while in flight, print to PSRAM for later dumping to SD card.
        {
            ram->print(logPrefix, false);
            ram->println(data, false);
        }
    }
}

void dataStageUpdate(int stage)
{
    dataStage = stage;
    if(dataStage > 4){
        ram->dumpFlightData();
        ram->dumpLogData();
    }
}