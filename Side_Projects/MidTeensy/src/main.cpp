#include <Arduino.h>

#include <stdint.h>

namespace
{

    constexpr uint32_t kUsbBaud = 115200;
    constexpr uint32_t kPacketBaud = 115200;
    constexpr uint32_t kPacketTimeoutMs = 100;
    constexpr size_t kMaxPayloadSize = 255;

    HardwareSerial &kRadioPort = Serial1;
    HardwareSerial &kAvionicsPort = Serial5;
    HardwareSerial &kAirbrakePort = Serial3;

    enum class RxState : uint8_t
    {
        WaitingForInput,
        WaitingForFinish,
    };

    struct PacketParser
    {
        const char *name = "";
        HardwareSerial *input = nullptr;
        RxState state = RxState::WaitingForInput;
        uint8_t packet[kMaxPayloadSize] = {};
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
        parser.state = RxState::WaitingForInput;
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
        Serial.write(parser.packet, packetSize);
        Serial.println();
    }

    void dropPacket(PacketParser &parser, const char *reason)
    {
        ++parser.droppedPackets;
        Serial.printf("%s drop: %s\n", parser.name, reason);
        resetParser(parser);
    }

    void beginPacket(PacketParser &parser, uint8_t byte)
    {
        parser.packet[0] = byte;
        parser.payloadBytesRead = 1;
        parser.state = RxState::WaitingForFinish;
        parser.lastByteAge = 0;
    }

    void consumeByte(PacketParser &parser, uint8_t byte)
    {
        parser.lastByteAge = 0;

        switch (parser.state)
        {
        case RxState::WaitingForInput:
            beginPacket(parser, byte);
            return;
        case RxState::WaitingForFinish:
            parser.packet[parser.payloadBytesRead] = byte;
            ++parser.payloadBytesRead;

            if (byte == '\n')
            {
                forwardPacket(parser, parser.payloadBytesRead);
                resetParser(parser);
            }
            if (parser.payloadBytesRead > kMaxPayloadSize)
            {
                dropPacket(parser, "payload too long");
                return;
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

        if (parser.state != RxState::WaitingForInput &&
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
