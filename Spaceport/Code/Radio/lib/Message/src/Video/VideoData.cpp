#include "VideoData.h"

VideoData::VideoData(uint8_t *data)
{
    // copy maxSize bytes from data into internal buffer
    memcpy(this->data, data, this->maxSize);
}

VideoData::VideoData(uint8_t *data, uint16_t sz)
{
    // make sure size is not too big
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;
    // copy size bytes into internal buffer
    memcpy(this->data, data, this->size);
}

uint16_t VideoData::encode(uint8_t *data, uint16_t sz)
{
    // make sure the provided array size is long enough
    if (sz >= this->maxSize)
    {
        // copy the data
        memcpy(data, this->data, this->maxSize);
        return this->maxSize;
    }
    return 0;
}

uint16_t VideoData::decode(uint8_t *data, uint16_t sz)
{
    // make sure size is not too big
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;
    // copy data into internal buffer
    memcpy(this->data, data, sz);
    return sz;
}