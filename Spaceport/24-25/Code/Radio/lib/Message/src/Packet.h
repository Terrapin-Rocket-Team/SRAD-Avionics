#ifndef PACKET_H
#define PACKET_H

#include "Arduino.h"

class Packet
{
public:
    uint8_t size = 0;
    uint8_t maxSize = 0;
    uint8_t *buf = new uint8_t[1];

    Packet() {};
    Packet(uint8_t packetSize);
    Packet(uint8_t *data, uint8_t packetSize);
    Packet(uint8_t *data, uint8_t sz, uint8_t packetSize);
    ~Packet();

    Packet *fill(uint8_t *data);
    Packet *fill(uint8_t *data, uint8_t sz);

    Packet *get(uint8_t *data);
    Packet *get(uint8_t *data, uint8_t &sz);
};

#endif