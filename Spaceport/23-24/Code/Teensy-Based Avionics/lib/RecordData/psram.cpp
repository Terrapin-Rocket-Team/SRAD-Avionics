#include "psram.h"

PSRAM::PSRAM()
{
    ready = false;
    dumped = false;
    launched = false;

    cursorStart = nullptr;
    cursorEnd = nullptr;
    memBegin = nullptr;
    memEnd = nullptr;
}

bool PSRAM::init(const char *csvHeader)
{
    uint8_t size = external_psram_size;
    memBegin = cursorStart = (char *)(0x70000000);
    memEnd = cursorEnd = memBegin + (size * 1048576);

    if (size > 0)
    {
        ready = true;
        println(csvHeader);
    }

    return ready;
}
void PSRAM::println(const char *data, bool atStart = true)
{
    print(data, atStart);
    print("\n", atStart);
}

// Write string to FRAM
void PSRAM::print(const char *data, bool atStart = true)
{
    if (ready)
        for (int i = 0; data[i] != '\0'; i++)
        {
            if (atStart)
            {
                *cursorStart = data[i];
                cursorStart++;
            }
            else
            {
                *cursorEnd = data[i];
                cursorEnd--;
            }
        }
}

// Dump FRAM to SD Card
bool PSRAM::dumpFlightData()
{
    if (isSDReady() && ready)
    {
        String curStr = "";
        float startTime = micros() / (1000000.0f);
        for (char *i = memBegin; i < cursorStart; i++)
        {
            char nextByte = *i;
            curStr = curStr + nextByte;
            if (nextByte == '\n')
            {
                logFile = sd.open(logFileName, FILE_WRITE);
                if (logFile)
                {
                    logFile.print(curStr);
                    logFile.close(); // close the file
                }
                // if (!isLaunchedPSRAM)
                // {
                //     if (previousRowsPSRAM.size() >= COMPACT_WALKBACK_COUNT)
                //     {
                //         previousRowsPSRAM.erase(previousRowsPSRAM.begin());
                //     }
                //     previousRowsPSRAM.push_back(String(curStr));
                // }
                curStr = "";
            }

            float curTime = micros() / (1000000.0f);
            if ((curTime - startTime) > PSRAM_DUMP_TIMEOUT)
            {
                // Serial.println("SD Timeout");
                return false;
            }
        }
        // Serial.println("Dumped");
    }

    cursorStart = memBegin;
    dumped = true;
    return true;
}

bool PSRAM::dumpLogData(){

}

// Returns whether the FRAM is initialized
bool PSRAM::isReady()
{
    return ready;
}

void PSRAM::markLiftoff()
{
    launched = true;
}