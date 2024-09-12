#include "GSData.h"

GSData::GSData(GSMessageType type) : type(type) {}

GSData::GSData(GSMessageType type, uint8_t *buf, uint16_t size) : type(type)
{
    this->fill(buf, size);
}

bool GSData::decodeHeader(uint32_t header, GSMessageType &type, uint16_t &size)
{
    uint8_t t = header >> 16;

    if (t > 0x04)
        return false; // error invalid message type

    type = (GSMessageType)t;
    size = header & 0x0000FFFF;
    return true;
}

uint16_t GSData::encode(uint8_t *data, uint16_t size)
{
    uint16_t pos = 0;
    if (size < this->size)
        return 0; // error not enough space for message

    // header
    data[pos++] = this->type;
    data[pos++] = this->size >> 8;
    data[pos++] = this->size & 0x00FF;

    // body
    memcpy(data + pos, this->buf, this->size);

    return pos + this->size;
}

uint16_t GSData::decode(uint8_t *data, uint16_t size)
{
    uint16_t pos = 0;
    if (size > this->maxSize)
        return 0; // error data too big

    // check for valid message type
    uint8_t t = data[pos++];
    // t is unsigned
    if (t > 0x04)
        return 0; // error invalid message type

    // header
    this->type = (GSMessageType)t;
    this->size = data[pos++] << 8;
    this->size += data[pos++];

    // body
    memcpy(this->buf, data + pos, size - pos);
    return size;
}

GSData *GSData::fill(uint8_t *buf, uint16_t size)
{
    // check if size is too big
    if (size <= this->maxDataSize)
    {
        // if it is fine fill the buffer
        this->size = size;
        memcpy(this->buf, buf, size);
    }
    else
    {
        // otherwise only copy dataMaxSize bytes
        this->size = this->maxDataSize;
        memcpy(this->buf, buf, this->maxDataSize);
    }
    return this;
}