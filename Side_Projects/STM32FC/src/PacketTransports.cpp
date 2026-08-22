#include "PacketTransports.h"

#include "Type_2GT.h"

void PacketTransports::addStream(Stream &stream)
{
    if (streamCount >= kMaxStreams)
        return;

    streams[streamCount++] = &stream;
}

void PacketTransports::setRadio(Type2GT *radioLink)
{
    radio = radioLink;
}

bool PacketTransports::send(const uint8_t *data, size_t len)
{
    if (data == nullptr || len == 0)
        return false;

    bool ok = true;

    for (size_t i = 0; i < streamCount; ++i)
    {
        Stream *stream = streams[i];
        if (stream == nullptr)
            continue;

        const size_t written = stream->write(data, len);
        ok = ok && (written == len);
    }

    if (radio != nullptr)
        ok = ok && (radio->transmit(data, len) == RADIOLIB_ERR_NONE);

    return ok;
}

bool PacketTransports::send(const avionics_packet::PacketBuffer &packet)
{
    return send(packet.data, packet.size);
}
