#ifndef GSDATA_H
#define GSDATA_H

#include "Data.h"

class GSData : public Data
{
public:
    // maximum total message size
    static const uint16_t maxSize = 0x0FFF;
    // size of the header
    static const uint8_t headerLen = 2;
    // maximum size of the data
    static const uint16_t maxDataSize = maxSize - headerLen;

    // type of message, first byte of header, 0x00 is default, max is 16
    uint8_t index = 0x00;
    // size of message, second and third bytes of header
    uint16_t size = 0x0000; // length of the message (2^12 = 4096 btyes should be enough)
    // buffer to store message data (not including header)
    uint8_t buf[maxDataSize] = {0}; // leave space for header

    // GSData default constructor
    GSData() {};

    static bool decodeHeader(uint16_t header, uint8_t &streamIndex, uint16_t &size);

    // GSData constructor
    // - type : the type of the message
    GSData(uint8_t streamIndex);

    // GSData constructor
    // - type : the type of the message
    // - buf : the data for the message
    // - size : the size of the data
    GSData(uint8_t streamIndex, uint8_t *buf, uint16_t size);

    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decode(uint8_t *data, uint16_t sz) override;

    // fill internal buffer with ```size``` bytes using the data in ```buf```
    GSData *fill(uint8_t *buf, uint16_t size);

    uint16_t toJSON(char *json, uint16_t sz, const char *streamName = "") override;
    uint16_t fromJSON(char *json, uint16_t sz, char *streamName) override;
};

#endif