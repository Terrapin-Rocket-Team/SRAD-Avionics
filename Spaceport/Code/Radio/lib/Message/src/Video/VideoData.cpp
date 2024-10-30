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

uint16_t VideoData::toJSON(char *json, uint16_t sz)
{
    uint16_t result = (uint16_t)snprintf(json, sz, "{\"type\": \"VideoData\", \"data\": {\"data\": [");

    if (result >= sz)
    {
        // output too large
        return 0;
    }

    // result should be the index of the \0

    // need to dynamically adjust the number of bytes added based on size
    for (int i = 0; i < this->size; i++)
    {
        if (result + 4 < sz)
        {
            int added = sprintf(json + result, "%d,", this->data[i]);
            if (added > 0 && added < sz)
                result += added;
            else
                return 0; // output too large
        }
        else
            return 0; // output too large
    }

    // result should be the index of \0

    // add closing braces
    // need to overwrite the last trailing comma
    if (result + 3 < sz)
    {
        json[result - 1] = ']';
        json[result] = '}';
        json[result + 1] = '}';
        json[result + 2] = '\0';

        // ran properly
        return result;
    }

    // output too large
    return 0;
}
