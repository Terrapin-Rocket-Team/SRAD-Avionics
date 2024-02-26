#ifndef PSRAM_H
#define PSRAM_H

#include "sdCard.h"
#include <vector>
#define PSRAM_DUMP_TIMEOUT 0.25 //in seconds

extern "C" uint8_t external_psram_size;
class PSRAM
{
public:
    PSRAM();
    bool init(const char *csvHeader);
    void print(const char *data, bool atStart = true);
    void println(const char *data, bool atStart = true);
    bool isReady();
    void markLiftoff();
    bool dumpFlightData();
    bool dumpLogData();

private:
    bool ready;
    bool dumped;
    bool launched;
    char *cursorStart;//for flight data
    char *cursorEnd;//for log data
    char *memBegin;//start of memory
    char *memEnd;//end of memory

};

#endif