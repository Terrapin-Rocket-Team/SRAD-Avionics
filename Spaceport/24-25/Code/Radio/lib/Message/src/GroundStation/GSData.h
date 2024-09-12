#ifndef GSDATA_H
#define GSDATA_H

#include "Data.h"

enum GSMessageType : uint8_t
{
    MSG_UNKWN = 0x00,
    MSG_TELEM = 0x01,
    MSG_CMD = 0x02,
    MSG_TEXT = 0x03,
    MSG_VIDEO = 0x04
};

class GSData : public Data
{
public:
    // maximum total message size
    static const uint16_t maxSize = 0xFFFF;
    // size of the header
    static const uint8_t headerLen = 3;
    // maximum size of the data
    static const uint16_t maxDataSize = maxSize - headerLen;

    // type of message, first byte of header
    GSMessageType type = MSG_UNKWN;
    // size of message, second and third bytes of header
    uint16_t size = 0x0000; // length of the message (2^16 should be enough)
    // buffer to store message data (not including header)
    uint8_t buf[maxDataSize] = {0}; // leave space for header

    // GSData default constructor
    GSData() {};

    static bool decodeHeader(uint32_t header, GSMessageType &type, uint16_t &size);

    // GSData constructor
    // - type : the type of the message
    GSData(GSMessageType type);

    // GSData constructor
    // - type : the type of the message
    // - buf : the data for the message
    // - size : the size of the data
    GSData(GSMessageType type, uint8_t *buf, uint16_t size);

    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decode(uint8_t *data, uint16_t sz) override;

    // fill internal buffer with ```size``` bytes using the data in ```buf```
    GSData *fill(uint8_t *buf, uint16_t size);
};

#endif