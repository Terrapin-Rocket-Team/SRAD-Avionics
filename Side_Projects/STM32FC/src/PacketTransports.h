#ifndef PACKET_TRANSPORTS_H
#define PACKET_TRANSPORTS_H

#include <Arduino.h>

#include "AvionicsPacketProtocol.h"

class Type2GT;

class PacketTransports
{
public:
    void addStream(Stream &stream);
    void setRadio(Type2GT *radioLink);

    bool send(const uint8_t *data, size_t len);
    bool send(const avionics_packet::PacketBuffer &packet);

private:
    static constexpr size_t kMaxStreams = 4;

    Stream *streams[kMaxStreams] = {};
    size_t streamCount = 0;
    Type2GT *radio = nullptr;
};

#endif // PACKET_TRANSPORTS_H
