#ifndef APRSDATA_H
#define APRSDATA_H

#include "../Data.h"

/*
APRS Configuration
- Callsign
- Tocall
- Path
- Type
- Symbol
- Overlay
*/
struct APRSConfig
{
    char callsign[8];
    char tocall[8];
    char path[10];
    char type;
    char symbol;
    char overlay;
};

enum APRSMessageType : char
{
    PositionWithoutTimestampWithAPRS = '=',
    PositionWithoutTimestampWithoutAPRS = '!',
    PositionWithTimestampWithAPRS = '@',
    PositionWithTimestampWithoutAPRS = '/',
    Status = '>',
    Query = '?',
    TextMessage = ':',
    Weather = '_',
    Telemetry = 'T',
    CurrentMicEData = '`',
};

class APRSData : public Data
{
public:
    APRSConfig config;
    APRSData(APRSConfig cfg) : config(cfg) {};
    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos);
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos);
    static void base10toBase91(uint8_t *str, uint16_t &pos, uint32_t val, int precision);
    static void base91toBase10(uint8_t *str, uint16_t &pos, uint32_t &val, int precision);
};

#endif