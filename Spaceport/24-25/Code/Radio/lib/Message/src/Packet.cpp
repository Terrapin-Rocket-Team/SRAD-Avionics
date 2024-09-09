#include "Packet.h"

Packet::Packet(uint8_t packetSize) : maxSize(packetSize)
{
    this->buf = new uint8_t[this->maxSize];
}

Packet::Packet(uint8_t *data, uint8_t packetSize) : size(packetSize), maxSize(packetSize)
{
    this->buf = new uint8_t[this->maxSize];
    memcpy(this->buf, data, this->maxSize);
}

Packet::Packet(uint8_t *data, uint8_t sz, uint8_t packetSize) : maxSize(packetSize)
{
    // size check
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;

    // copy data
    this->buf = new uint8_t[this->maxSize];
    memcpy(this->buf, data, this->size);
}

Packet::~Packet()
{
    delete[] this->buf;
}

Packet *Packet::fill(uint8_t *data)
{
    this->size = this->maxSize;
    memcpy(this->buf, data, this->maxSize);
    return this;
}

Packet *Packet::fill(uint8_t *data, uint8_t sz)
{
    // size check
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;

    // copy bytes
    memcpy(this->buf, data, this->size);
    return this;
}

Packet *Packet::get(uint8_t *data)
{
    memcpy(data, this->buf, this->size);
    return this;
}

Packet *Packet::get(uint8_t *data, uint8_t &sz)
{
    sz = this->size;
    memcpy(data, this->buf, this->size);
    return this;
}