#include "VideoData.h"

VideoData::VideoData(uint8_t *data)
{
    memcpy(this->data, data, this->maxSize);
}

VideoData::VideoData(uint8_t *data, uint16_t sz)
{
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;
    memcpy(this->data, data, this->size);
}

uint16_t VideoData::encode(uint8_t *data, uint16_t sz)
{
    if (sz >= this->maxSize)
    {
        memcpy(data, this->data, this->maxSize);
        return this->maxSize;
    }
    return 0;
}

uint16_t VideoData::decode(uint8_t *data, uint16_t sz)
{
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;
    memcpy(this->data, data, sz);
    return sz;
}