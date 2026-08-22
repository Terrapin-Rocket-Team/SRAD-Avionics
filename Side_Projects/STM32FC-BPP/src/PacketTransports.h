#ifndef PACKET_TRANSPORTS_H
#define PACKET_TRANSPORTS_H

#include "AvionicsPacketProtocol.h"

class Type2GT;

class PacketTransports
{
public:
    void setRadio(Type2GT *radioLink);

    bool send(const uint8_t *data, size_t len);
    bool send(const avionics_packet::PacketBuffer &packet);

private:
    Type2GT *radio = nullptr;
};

#endif // PACKET_TRANSPORTS_H
