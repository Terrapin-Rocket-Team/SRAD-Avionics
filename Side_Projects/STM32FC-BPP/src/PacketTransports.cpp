#include "PacketTransports.h"

#include "Type_2GT.h"

void PacketTransports::setRadio(Type2GT *radioLink)
{
    radio = radioLink;
}

bool PacketTransports::send(const uint8_t *data, size_t len)
{
    if (data == nullptr || len == 0)
        return false;

    return radio != nullptr && (radio->transmit(data, len) == RADIOLIB_ERR_NONE);
}

bool PacketTransports::send(const avionics_packet::PacketBuffer &packet)
{
    return send(packet.data, packet.size);
}
