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

/*
APRS Message types
- PositionWithoutTimestampWithAPRS '=',
- PositionWithoutTimestampWithoutAPRS '!',
- PositionWithTimestampWithAPRS '@',
- PositionWithTimestampWithoutAPRS '/',
- Status '>',
- Query '?',
- TextMessage ':',
- Weather '_',
- Telemetry 'T',
- CurrentMicEData '`',
*/
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
    // the configuration for this message
    // Note: modified when message is decoded
    APRSConfig config;

    // APRSData constructor
    // - cfg : the APRS config to use
    APRSData(APRSConfig cfg) : config(cfg) {};

    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos);
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos);

    // convert a number to a base 91 number represented by ascii characters
    // - str : the string to add the number to, must be long enough to hold ```precision``` more characters
    // - pos : the position to in ```str``` to add the number to
    // - val : the value of the number to add
    // - precision : the number of base91 digits to use
    static void numtoBase91(uint8_t *str, uint16_t &pos, uint32_t val, int precision);
    // convert a base 91 number represented by ascii characters to a number
    // - str : the string to take the number from, must contain at least ```precision``` more characters
    // - pos : the position to in ```str``` to start taking the number from
    // - val : will be set to the value of the number
    // - precision : the number of base91 digits to use
    static void base91toNum(uint8_t *str, uint16_t &pos, uint32_t &val, int precision);
};

#endif