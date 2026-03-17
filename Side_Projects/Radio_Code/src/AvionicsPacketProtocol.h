#ifndef AVIONICS_PACKET_PROTOCOL_H
#define AVIONICS_PACKET_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

namespace avionics_packet
{
enum class MessageType : uint8_t
{
    ABTELEM = 0,
    AVITELEM = 1,
    ABCMD = 2,
    AVICMD = 3,
    LIVECMD = 4,
    NOSEVIDCMD = 5,
    ABVIDCMD = 6,
};

constexpr size_t kPacketHeaderSize = 2;
constexpr size_t kMaxPayloadSize = 64;
constexpr size_t kMaxPacketSize = kPacketHeaderSize + kMaxPayloadSize;

constexpr size_t aviTelemetryPayloadSize()
{
    return 2 + 2 + 2 + 2 + 8 + 2 + 8;
}

constexpr size_t abTelemetryPayloadSize()
{
    return 2 + 2 + 2 + 2 + 8 + 2 + 2 + 2 + 2 + 2;
}

inline bool isKnownMessageType(uint8_t rawType)
{
    return rawType <= static_cast<uint8_t>(MessageType::ABVIDCMD);
}

inline size_t fixedPayloadSize(MessageType type)
{
    switch (type)
    {
    case MessageType::ABTELEM:
        return abTelemetryPayloadSize();
    case MessageType::AVITELEM:
        return aviTelemetryPayloadSize();
    default:
        return 0;
    }
}

inline bool packetLengthLooksValid(const uint8_t *packet, size_t packetSize)
{
    if (packet == nullptr || packetSize < kPacketHeaderSize)
        return false;

    const size_t payloadSize = packet[1];
    if (packetSize != kPacketHeaderSize + payloadSize)
        return false;

    if (!isKnownMessageType(packet[0]))
        return false;

    const size_t fixedSize = fixedPayloadSize(static_cast<MessageType>(packet[0]));
    return fixedSize == 0 || fixedSize == payloadSize;
}
} // namespace avionics_packet

#endif // AVIONICS_PACKET_PROTOCOL_H
