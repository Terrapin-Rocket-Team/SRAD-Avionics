#include "psram.h"

PSRAM::PSRAM()
{
    ready = false;
    dumped = false;

    cursorStart = nullptr;
    cursorEnd = nullptr;
    memBegin = nullptr;
    memEnd = nullptr;
}

bool PSRAM::init()
{
    uint8_t size = external_psram_size;
    memBegin = cursorStart = (char *)(0x70000000);
    memEnd = cursorEnd = memBegin + (size * 1048576);

    if (size > 0)
    {
        ready = true;
    }

    return ready;
}
void PSRAM::println(const char *data, bool atStart)
{
    if (ready)
    {
        print(data, atStart);
        print("\n", atStart);
    }
}

// Write string to FRAM
void PSRAM::print(const char *data, bool atStart)
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

// Dump FRAM to SD Card. Only to be called after flight has landed or timeout is reached.
bool PSRAM::dumpFlightData()
{
    if (isSDReady() && ready)
    {
        flightDataFile = sd.open(flightDataFileName, FILE_WRITE);
        if (flightDataFile)
        {
            flightDataFile.write(memBegin, cursorStart - memBegin);
            flightDataFile.close();
        }
        else return false;
    } else return false;

    cursorStart = memBegin;
    return true;
}

bool PSRAM::dumpLogData()
{ // more complicated because data is stored in reverse order
    if (!isSDReady() || !ready)
        return false;

    char buffer[2048]; // large buffer but the Teensy should be able to handle it. Buffer should be a multiple of 512 for SD card efficiency.
    char *writeCursor = memEnd;
    int i = 0;
    logFile = sd.open(logFileName, FILE_WRITE);

    if (!logFile)
        return false;
    
    while (writeCursor >= cursorEnd)
    {

        for (i = 0; i < 2048; i++)
        {
            if (writeCursor <= cursorEnd)
            {
                buffer[i] = '\0';
                writeCursor--;
                break;
            }
            buffer[i] = *writeCursor--;
        }
        logFile.write(buffer, i);
    }

    logFile.close();
    cursorEnd = memEnd;
    dumped = true;
    return true;
}

// Returns whether the FRAM is initialized
bool PSRAM::isReady()
{
    return ready;
}

int PSRAM::getFreeSpace()
{
    return cursorEnd - cursorStart;
}