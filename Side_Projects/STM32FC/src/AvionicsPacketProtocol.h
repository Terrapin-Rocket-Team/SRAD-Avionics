#ifndef AVIONICS_PACKET_PROTOCOL_H
#define AVIONICS_PACKET_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include <cmath>
#include <cstring>
#include <limits>

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

constexpr size_t kPacketHeaderSize = 2; // type + payload length
constexpr size_t kMaxPayloadSize = 64;
constexpr size_t kMaxPacketSize = kPacketHeaderSize + kMaxPayloadSize;
constexpr uint16_t kUnknownBatteryCentivolts = 0xFFFF;
constexpr int32_t kUnknownGpsE7 = std::numeric_limits<int32_t>::min();
constexpr int16_t kUnknownSigned16 = std::numeric_limits<int16_t>::min();

struct PacketBuffer
{
    uint8_t data[kMaxPacketSize] = {};
    size_t size = 0;
};

struct AviTelemetry
{
    float positionZFeet = 0.0f;
    float velocityZMs = 0.0f;
    float accelZMs2 = 0.0f;
    bool hasBaroAgl = false;
    float baroAglFeet = 0.0f;
    float quatW = 1.0f;
    float quatX = 0.0f;
    float quatY = 0.0f;
    float quatZ = 0.0f;
    bool hasBattery = false;
    float batteryVolts = 0.0f;
    bool hasGps = false;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
};

struct AbTelemetry
{
    float positionZFeet = 0.0f;
    float velocityZMs = 0.0f;
    float accelZMs2 = 0.0f;
    bool hasBaroAgl = false;
    float baroAglFeet = 0.0f;
    float quatW = 1.0f;
    float quatX = 0.0f;
    float quatY = 0.0f;
    float quatZ = 0.0f;
    bool hasBattery = false;
    float batteryVolts = 0.0f;
    bool hasMotorBattery = false;
    float motorBatteryVolts = 0.0f;
    float desiredAngleDeg = 0.0f;
    float actualAngleDeg = 0.0f;
    float predictedApogeeFeet = 0.0f;
};

constexpr size_t aviTelemetryPayloadSize()
{
    return 2 + 2 + 2 + 2 + 8 + 2 + 8;
}

constexpr size_t aviTelemetryPacketSize()
{
    return kPacketHeaderSize + aviTelemetryPayloadSize();
}

constexpr size_t abTelemetryPayloadSize()
{
    return 2 + 2 + 2 + 2 + 8 + 2 + 2 + 2 + 2 + 2;
}

constexpr size_t abTelemetryPacketSize()
{
    return kPacketHeaderSize + abTelemetryPayloadSize();
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

inline long long clampSigned(long long value, long long minValue, long long maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

inline unsigned long long clampUnsigned(unsigned long long value, unsigned long long maxValue)
{
    return value > maxValue ? maxValue : value;
}

inline int16_t quantizeSigned(float value, float scale)
{
    const long long scaled = llround(static_cast<double>(value) * static_cast<double>(scale));
    return static_cast<int16_t>(
        clampSigned(scaled, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()));
}

inline uint16_t quantizeUnsigned(float value, float scale)
{
    if (!std::isfinite(value) || value < 0.0f)
        return 0;

    const unsigned long long scaled = static_cast<unsigned long long>(
        llround(static_cast<double>(value) * static_cast<double>(scale)));
    return static_cast<uint16_t>(clampUnsigned(scaled, std::numeric_limits<uint16_t>::max() - 1ULL));
}

inline int16_t quantizeQuaternionComponent(float value)
{
    const float clamped = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
    return quantizeSigned(clamped, 32767.0f);
}

inline float decodeQuaternionComponent(int16_t raw)
{
    return static_cast<float>(raw) / 32767.0f;
}

inline int32_t quantizeGpsDegrees(double value)
{
    if (!std::isfinite(value))
        return kUnknownGpsE7;

    const double scaled = std::round(value * 10000000.0);
    constexpr double kI32Min = static_cast<double>(std::numeric_limits<int32_t>::min() + 1);
    constexpr double kI32Max = static_cast<double>(std::numeric_limits<int32_t>::max());
    const double clamped = scaled < kI32Min ? kI32Min : (scaled > kI32Max ? kI32Max : scaled);
    return static_cast<int32_t>(clamped);
}

inline void writeU16LE(uint8_t *dst, uint16_t value)
{
    dst[0] = static_cast<uint8_t>(value & 0xFF);
    dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

inline void writeI16LE(uint8_t *dst, int16_t value)
{
    writeU16LE(dst, static_cast<uint16_t>(value));
}

inline void writeI32LE(uint8_t *dst, int32_t value)
{
    const uint32_t raw = static_cast<uint32_t>(value);
    dst[0] = static_cast<uint8_t>(raw & 0xFF);
    dst[1] = static_cast<uint8_t>((raw >> 8) & 0xFF);
    dst[2] = static_cast<uint8_t>((raw >> 16) & 0xFF);
    dst[3] = static_cast<uint8_t>((raw >> 24) & 0xFF);
}

inline uint16_t readU16LE(const uint8_t *src)
{
    return static_cast<uint16_t>(src[0]) |
           (static_cast<uint16_t>(src[1]) << 8);
}

inline int16_t readI16LE(const uint8_t *src)
{
    return static_cast<int16_t>(readU16LE(src));
}

inline int32_t readI32LE(const uint8_t *src)
{
    return static_cast<int32_t>(static_cast<uint32_t>(src[0]) |
                                (static_cast<uint32_t>(src[1]) << 8) |
                                (static_cast<uint32_t>(src[2]) << 16) |
                                (static_cast<uint32_t>(src[3]) << 24));
}

inline bool buildPacket(MessageType type, const uint8_t *payload, size_t payloadSize, PacketBuffer &packet)
{
    if (payloadSize > kMaxPayloadSize)
        return false;

    packet.data[0] = static_cast<uint8_t>(type);
    packet.data[1] = static_cast<uint8_t>(payloadSize);

    if (payloadSize > 0 && payload != nullptr)
        std::memcpy(&packet.data[kPacketHeaderSize], payload, payloadSize);

    packet.size = kPacketHeaderSize + payloadSize;
    return true;
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

inline bool encodeAviTelemetry(const AviTelemetry &input, PacketBuffer &packet)
{
    uint8_t payload[aviTelemetryPayloadSize()] = {};
    size_t offset = 0;

    writeI16LE(&payload[offset], quantizeSigned(input.positionZFeet, 1.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.velocityZMs, 10.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.accelZMs2, 10.0f));
    offset += 2;

    const int16_t baroAglFeet = input.hasBaroAgl ? quantizeSigned(input.baroAglFeet, 1.0f) : kUnknownSigned16;
    writeI16LE(&payload[offset], baroAglFeet);
    offset += 2;

    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatW));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatX));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatY));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatZ));
    offset += 2;

    const uint16_t batteryCentivolts = input.hasBattery
                                           ? quantizeUnsigned(input.batteryVolts, 100.0f)
                                           : kUnknownBatteryCentivolts;
    writeU16LE(&payload[offset], batteryCentivolts);
    offset += 2;

    const int32_t latitudeE7 = input.hasGps ? quantizeGpsDegrees(input.latitudeDeg) : kUnknownGpsE7;
    const int32_t longitudeE7 = input.hasGps ? quantizeGpsDegrees(input.longitudeDeg) : kUnknownGpsE7;
    writeI32LE(&payload[offset], latitudeE7);
    offset += 4;
    writeI32LE(&payload[offset], longitudeE7);
    offset += 4;

    return buildPacket(MessageType::AVITELEM, payload, offset, packet);
}

inline bool encodeAbTelemetry(const AbTelemetry &input, PacketBuffer &packet)
{
    uint8_t payload[abTelemetryPayloadSize()] = {};
    size_t offset = 0;

    writeI16LE(&payload[offset], quantizeSigned(input.positionZFeet, 1.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.velocityZMs, 10.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.accelZMs2, 10.0f));
    offset += 2;

    const int16_t baroAglFeet = input.hasBaroAgl ? quantizeSigned(input.baroAglFeet, 1.0f) : kUnknownSigned16;
    writeI16LE(&payload[offset], baroAglFeet);
    offset += 2;

    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatW));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatX));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatY));
    offset += 2;
    writeI16LE(&payload[offset], quantizeQuaternionComponent(input.quatZ));
    offset += 2;

    const uint16_t batteryCentivolts = input.hasBattery
                                           ? quantizeUnsigned(input.batteryVolts, 100.0f)
                                           : kUnknownBatteryCentivolts;
    writeU16LE(&payload[offset], batteryCentivolts);
    offset += 2;

    const uint16_t motorBatteryCentivolts = input.hasMotorBattery
                                                ? quantizeUnsigned(input.motorBatteryVolts, 100.0f)
                                                : kUnknownBatteryCentivolts;
    writeU16LE(&payload[offset], motorBatteryCentivolts);
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.desiredAngleDeg, 10.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.actualAngleDeg, 10.0f));
    offset += 2;

    writeI16LE(&payload[offset], quantizeSigned(input.predictedApogeeFeet, 1.0f));
    offset += 2;

    return buildPacket(MessageType::ABTELEM, payload, offset, packet);
}

inline bool decodeAviTelemetry(const uint8_t *packet, size_t packetSize, AviTelemetry &output)
{
    if (!packetLengthLooksValid(packet, packetSize))
        return false;

    if (static_cast<MessageType>(packet[0]) != MessageType::AVITELEM)
        return false;

    const uint8_t *payload = &packet[kPacketHeaderSize];
    size_t offset = 0;

    output.positionZFeet = static_cast<float>(readI16LE(&payload[offset]));
    offset += 2;

    output.velocityZMs = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    output.accelZMs2 = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    const int16_t baroAglFeet = readI16LE(&payload[offset]);
    offset += 2;
    output.hasBaroAgl = baroAglFeet != kUnknownSigned16;
    output.baroAglFeet = output.hasBaroAgl ? static_cast<float>(baroAglFeet) : 0.0f;

    output.quatW = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatX = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatY = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatZ = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;

    const uint16_t batteryCentivolts = readU16LE(&payload[offset]);
    offset += 2;
    output.hasBattery = batteryCentivolts != kUnknownBatteryCentivolts;
    output.batteryVolts = output.hasBattery ? static_cast<float>(batteryCentivolts) / 100.0f : 0.0f;

    const int32_t latitudeE7 = readI32LE(&payload[offset]);
    offset += 4;
    const int32_t longitudeE7 = readI32LE(&payload[offset]);
    offset += 4;

    output.hasGps = latitudeE7 != kUnknownGpsE7 && longitudeE7 != kUnknownGpsE7;
    output.latitudeDeg = output.hasGps ? static_cast<double>(latitudeE7) / 10000000.0 : 0.0;
    output.longitudeDeg = output.hasGps ? static_cast<double>(longitudeE7) / 10000000.0 : 0.0;

    return offset == aviTelemetryPayloadSize();
}

inline bool decodeAbTelemetry(const uint8_t *packet, size_t packetSize, AbTelemetry &output)
{
    if (!packetLengthLooksValid(packet, packetSize))
        return false;

    if (static_cast<MessageType>(packet[0]) != MessageType::ABTELEM)
        return false;

    const uint8_t *payload = &packet[kPacketHeaderSize];
    size_t offset = 0;

    output.positionZFeet = static_cast<float>(readI16LE(&payload[offset]));
    offset += 2;

    output.velocityZMs = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    output.accelZMs2 = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    const int16_t baroAglFeet = readI16LE(&payload[offset]);
    offset += 2;
    output.hasBaroAgl = baroAglFeet != kUnknownSigned16;
    output.baroAglFeet = output.hasBaroAgl ? static_cast<float>(baroAglFeet) : 0.0f;

    output.quatW = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatX = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatY = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;
    output.quatZ = decodeQuaternionComponent(readI16LE(&payload[offset]));
    offset += 2;

    const uint16_t batteryCentivolts = readU16LE(&payload[offset]);
    offset += 2;
    output.hasBattery = batteryCentivolts != kUnknownBatteryCentivolts;
    output.batteryVolts = output.hasBattery ? static_cast<float>(batteryCentivolts) / 100.0f : 0.0f;

    const uint16_t motorBatteryCentivolts = readU16LE(&payload[offset]);
    offset += 2;
    output.hasMotorBattery = motorBatteryCentivolts != kUnknownBatteryCentivolts;
    output.motorBatteryVolts = output.hasMotorBattery ? static_cast<float>(motorBatteryCentivolts) / 100.0f : 0.0f;

    output.desiredAngleDeg = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    output.actualAngleDeg = static_cast<float>(readI16LE(&payload[offset])) / 10.0f;
    offset += 2;

    output.predictedApogeeFeet = static_cast<float>(readI16LE(&payload[offset]));
    offset += 2;

    return offset == abTelemetryPayloadSize();
}

} // namespace avionics_packet

#endif // AVIONICS_PACKET_PROTOCOL_H
