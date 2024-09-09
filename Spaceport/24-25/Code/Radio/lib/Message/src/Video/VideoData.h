#ifndef VIDEODATA_H
#define VIDEODATA_H

#include "../Data.h"

class VideoData : public Data
{
public:
    static const uint16_t maxSize = 2000;
    uint16_t size;
    uint8_t data[maxSize] = {0};

    // constructors
    VideoData() {};
    VideoData(uint8_t *data);
    VideoData(uint8_t *data, uint16_t sz);
    // encode/decode

    // encode the data stored in the ```Data``` object and place the result in ```data```, ```sz``` is the max size of ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object, ```sz``` is the number of bytes from ```data``` to decode
    uint16_t decode(uint8_t *data, uint16_t sz) override;
};

#endif