#include "Message.h"

// public methods

// constructors

Message::Message(MessageType type, uint8_t data[maxSize], uint16_t sz) : type()
{
    if (sz > this->maxSize)
        this->size = this->maxSize;
    else
        this->size = sz;

    memcpy(this->buf, data, this->size);
}

Message::Message(MessageType type, Data *data) : type()
{
    // encode the given data
    this->size = data->encode(this->buf, this->maxSize);
}

// destructor

Message *Message::encode(Data *data)
{
    this->size = data->encode(this->buf, this->maxSize);
    return this;
}

Message *Message::decode(Data *data)
{
    data->decode(this->buf, this->size);
    return this;
}

Message *Message::clear()
{
    memset(this->buf, 0, this->maxSize);
    this->size = 0;
    return this;
}

Message *Message::addPacket(Packet *p)
{
    if (this->size + p->size <= this->maxSize)
    {
        p->get(this->buf + this->size);
        this->size += p->size;
    }
    return this;
}

Message *Message::popPacket(Packet *p)
{
    if (this->size - p->maxSize > 0)
    {
        p->fill(this->buf + this->size - p->maxSize);
        this->size -= p->maxSize;
    }
    else if (this->size > 0)
    {
        p->fill(this->buf, this->size);
        this->size = 0;
    }
    return this;
}

Message *Message::shiftPacket(Packet *p)
{
    if (this->size - p->maxSize > 0)
    {
        p->fill(this->buf);
        this->size -= p->maxSize;
        memcpy(this->buf, this->buf + p->maxSize, this->size);
    }
    else if (this->size > 0)
    {
        p->fill(this->buf, this->size);
        this->size = 0;
    }
    return this;
}

Message *Message::getPacket(Packet *p, uint16_t index)
{
    if (p->maxSize * (index + 1) < this->size)
    {
        p->fill(this->buf + (p->maxSize * index));
    }
    else if (this->size - (p->maxSize * index) > 0)
    {
        p->fill(this->buf + (p->maxSize * index), this->size - (p->maxSize * index));
    }
    return this;
}

// utility methods

Message *Message::fill(uint8_t *data, uint16_t sz)
{
    if (sz > this->maxSize)
        memcpy(this->buf, data, maxSize);
    else
        memcpy(this->buf, data, sz);
    return this;
}

Message *Message::fill(uint8_t *data, uint16_t start, uint16_t end)
{
    if (end - start > this->maxSize)
        memcpy(this->buf, data + start, end - start);
    else
        memcpy(this->buf, data + start, end - start);
    return this;
}

Message *Message::get(uint8_t *data)
{
    memcpy(data, this->buf, this->size);
    return this;
}

Message *Message::get(uint8_t *data, uint16_t &sz)
{
    this->get(data, sz, 0, sz);
    return this;
}

Message *Message::get(uint8_t *data, uint16_t &sz, uint16_t start)
{
    this->get(data, sz, start, sz);
    return this;
}

Message *Message::get(uint8_t *data, uint16_t &sz, uint16_t start, uint16_t end)
{
    if (start < end && end - start > this->size)
    {
        sz = this->size - start;
        memcpy(data, this->buf + start, sz);
    }
    else if (start < end)
    {
        sz = end - start;
        memcpy(data, this->buf + start, sz);
    }
    return this;
}