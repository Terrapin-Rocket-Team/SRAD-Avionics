#include "RecordData.h"

int PRE_FLIGHT_DATA_DUMP_DURATION = 60;  // in seconds
int PRE_FLIGHT_TIME_SINCE_LAST_DUMP = 0; // in seconds
int PRE_FLIGHT_TIME_OF_LAST_DUMP = 0;    // in seconds

void recordData(char *data, int stage)
{
    if (stage == 0)
    {
        dataToPSRAM(data);
        PRE_FLIGHT_TIME_SINCE_LAST_DUMP = (millis() / 1000) - PRE_FLIGHT_TIME_OF_LAST_DUMP;
        if (PRE_FLIGHT_TIME_SINCE_LAST_DUMP > PRE_FLIGHT_DATA_DUMP_DURATION)
        {
            String dumped = PSRAMDumpToSD();
            if (dumped == "Dumped")
            {
                resetPSRAMDumpStatus();
                PRE_FLIGHT_TIME_OF_LAST_DUMP = millis() / 1000;
            }
        }
    }
    else if (stage > 4)
    {
        if (!isPSRAMDumped())
            PSRAMDumpToSD();
    }
    else
    {
        psramMarkLiftoff(); // TODO this should only have to run the first time
        dataToPSRAM(data);
    }
}

void dataToPSRAM(char *data)
{
    if (isPSRAMReady())
        psramPrintln(data);
}