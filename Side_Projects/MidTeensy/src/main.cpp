#include <Arduino.h>

#include <stdint.h>

namespace
{

constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kPacketBaud = 115200;
constexpr uint32_t kPacketTimeoutMs = 100;
constexpr size_t kHeaderSize = 2;
constexpr size_t kMaxPayloadSize = 255;
constexpr size_t kMaxPacketSize = kHeaderSize + kMaxPayloadSize;

HardwareSerial &kRadioPort = Serial1;
HardwareSerial &kAvionicsPort = Serial3;
HardwareSerial &kAirbrakePort = Serial5;

enum class RxState : uint8_t
{
    WaitingForType,
    WaitingForLength,
    WaitingForPayload,
};

struct PacketParser
{
    const char *name = "";
    HardwareSerial *input = nullptr;
    RxState state = RxState::WaitingForType;
    uint8_t packet[kMaxPacketSize] = {};
    uint8_t expectedPayloadLength = 0;
    uint8_t payloadBytesRead = 0;
    elapsedMillis lastByteAge = 0;
    uint32_t forwardedPackets = 0;
    uint32_t droppedPackets = 0;
};

PacketParser avionicsParser = {
    "AVIONICS",
    &kAvionicsPort,
};

PacketParser airbrakeParser = {
    "AIRBRAKE",
    &kAirbrakePort,
};

elapsedMillis statusAge;

void resetParser(PacketParser &parser)
{
    parser.state = RxState::WaitingForType;
    parser.expectedPayloadLength = 0;
    parser.payloadBytesRead = 0;
    parser.lastByteAge = 0;
}

void printHexBytes(const uint8_t *bytes, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        Serial.printf("%02X", bytes[i]);
        if ((i + 1U) < length)
        {
            Serial.print(' ');
        }
    }
}

void forwardPacket(PacketParser &parser, size_t packetSize)
{
    kRadioPort.write(parser.packet, packetSize);
    ++parser.forwardedPackets;

    Serial.printf("%s -> RADIO len=%u raw=",
                  parser.name,
                  static_cast<unsigned>(packetSize));
    printHexBytes(parser.packet, packetSize);
    Serial.println();
}

void dropPacket(PacketParser &parser, const char *reason)
{
    ++parser.droppedPackets;
    Serial.printf("%s drop: %s\n", parser.name, reason);
    resetParser(parser);
}

void beginPacket(PacketParser &parser, uint8_t type)
{
    parser.packet[0] = type;
    parser.state = RxState::WaitingForLength;
    parser.lastByteAge = 0;
}

void consumeByte(PacketParser &parser, uint8_t byte)
{
    parser.lastByteAge = 0;

    switch (parser.state)
    {
    case RxState::WaitingForType:
        beginPacket(parser, byte);
        return;

    case RxState::WaitingForLength:
        parser.packet[1] = byte;
        parser.expectedPayloadLength = byte;
        parser.payloadBytesRead = 0;

        if (byte > kMaxPayloadSize)
        {
            dropPacket(parser, "payload too long");
            return;
        }

        if (byte == 0)
        {
            forwardPacket(parser, kHeaderSize);
            resetParser(parser);
            return;
        }

        parser.state = RxState::WaitingForPayload;
        return;

    case RxState::WaitingForPayload:
        parser.packet[kHeaderSize + parser.payloadBytesRead] = byte;
        ++parser.payloadBytesRead;

        if (parser.payloadBytesRead >= parser.expectedPayloadLength)
        {
            forwardPacket(parser, kHeaderSize + parser.expectedPayloadLength);
            resetParser(parser);
        }
        return;
    }
}

void serviceInput(PacketParser &parser)
{
    while (parser.input->available() > 0)
    {
        const int incoming = parser.input->read();
        if (incoming < 0)
        {
            continue;
        }

        consumeByte(parser, static_cast<uint8_t>(incoming));
    }

    if (parser.state != RxState::WaitingForType &&
        parser.lastByteAge > kPacketTimeoutMs)
    {
        dropPacket(parser, "timeout while receiving packet");
    }
}

void waitForUsbSerial()
{
    const uint32_t start = millis();
    while (!Serial && (millis() - start) < 3000)
    {
        delay(10);
    }
}

} // namespace

void setup()
{
    Serial.begin(kUsbBaud);
    waitForUsbSerial();

    kRadioPort.begin(kPacketBaud);
    kAvionicsPort.begin(kPacketBaud);
    kAirbrakePort.begin(kPacketBaud);

    resetParser(avionicsParser);
    resetParser(airbrakeParser);
    statusAge = 0;

    Serial.println("Packet router ready");
    Serial.println("Serial3  -> Serial1 (Avionics -> Radio)");
    Serial.println("Serial5  -> Serial1 (Airbrake -> Radio)");
    Serial.println("Packet format: [type][length][payload...]");
}

void loop()
{
    serviceInput(avionicsParser);
    serviceInput(airbrakeParser);

    if (statusAge > 5000)
    {
        statusAge = 0;
        Serial.printf("Stats: AVIONICS ok=%lu drop=%lu | AIRBRAKE ok=%lu drop=%lu\n",
                      static_cast<unsigned long>(avionicsParser.forwardedPackets),
                      static_cast<unsigned long>(avionicsParser.droppedPackets),
                      static_cast<unsigned long>(airbrakeParser.forwardedPackets),
                      static_cast<unsigned long>(airbrakeParser.droppedPackets));
    }
}
