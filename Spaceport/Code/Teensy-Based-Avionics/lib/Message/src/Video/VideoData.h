#ifndef VIDEODATA_H
#define VIDEODATA_H

#include "../Data.h"

class VideoData : public Data
{
public:
    // the maximum size of the data
    static const uint16_t maxSize = 2000;
    // the size of the data
    uint16_t size = 0;
    // buffer to store the data
    uint8_t data[maxSize] = {0};

    // VideoData default constructor
    VideoData() {};

    // VideoData constructor
    // - data : the data to be encoded, assumed to be ```maxSize```
    VideoData(uint8_t *data);

    // VideoData constructor
    // - data : the data to be encoded
    // - sz : the size of the data
    VideoData(uint8_t *data, uint16_t sz);

    // encode the data stored in the ```Data``` object and place the result in ```data```, ```sz``` is the max size of ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object, ```sz``` is the number of bytes from ```data``` to decode
    uint16_t decode(uint8_t *data, uint16_t sz) override;

    uint16_t toJSON(char *json, uint16_t sz, const char *streamName = "") override;
    uint16_t fromJSON(char *json, uint16_t sz, char *streamName) override;
};

#endif