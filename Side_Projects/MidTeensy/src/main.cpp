#include <Arduino.h>

#include <ctype.h>
#include <stdint.h>

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

constexpr size_t kPacketHeaderSize = 2;
constexpr size_t kMaxPayloadSize = 64;
constexpr size_t kMaxPacketSize = kPacketHeaderSize + kMaxPayloadSize;
constexpr uint16_t kUnknownBatteryCentivolts = 0xFFFF;
constexpr int32_t kUnknownGpsE7 = std::numeric_limits<int32_t>::min();
constexpr int16_t kUnknownSigned16 = std::numeric_limits<int16_t>::min();

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

inline float decodeQuaternionComponent(int16_t raw)
{
    return static_cast<float>(raw) / 32767.0f;
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

namespace
{

constexpr uint32_t kUsbBaud = 115200;
constexpr uint32_t kTelemetryBaud = 115200;
constexpr uint32_t kPacketTimeoutMs = 100;
constexpr size_t kUsbCommandBufferSize = 128;

HardwareSerial &kTelemetryPort = Serial1;

enum class RxState : uint8_t
{
    WaitingForType,
    WaitingForLength,
    WaitingForPayload,
};

struct PacketParser
{
    RxState state = RxState::WaitingForType;
    uint8_t packet[avionics_packet::kMaxPacketSize] = {};
    uint8_t expectedPayloadLength = 0;
    uint8_t payloadBytesRead = 0;
    elapsedMillis lastByteAge = 0;
};

PacketParser parser;
elapsedMillis heartbeatAge;
char usbCommandBuffer[kUsbCommandBufferSize] = {};
size_t usbCommandLength = 0;
bool decodePacketsEnabled = true;
bool rawMirrorEnabled = false;

const char *messageTypeName(uint8_t rawType)
{
    using avionics_packet::MessageType;

    switch (static_cast<MessageType>(rawType))
    {
    case MessageType::ABTELEM:
        return "ABTELEM";
    case MessageType::AVITELEM:
        return "AVITELEM";
    case MessageType::ABCMD:
        return "ABCMD";
    case MessageType::AVICMD:
        return "AVICMD";
    case MessageType::LIVECMD:
        return "LIVECMD";
    case MessageType::NOSEVIDCMD:
        return "NOSEVIDCMD";
    case MessageType::ABVIDCMD:
        return "ABVIDCMD";
    default:
        return "UNKNOWN";
    }
}

void resetParser()
{
    parser.state = RxState::WaitingForType;
    parser.expectedPayloadLength = 0;
    parser.payloadBytesRead = 0;
    parser.lastByteAge = 0;
}

void printHexBytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        Serial.printf("%02X", data[i]);
        if ((i + 1U) < size)
            Serial.print(' ');
    }
}

bool isPrintableAscii(uint8_t byte)
{
    return byte >= 32U && byte <= 126U;
}

bool payloadToAscii(const uint8_t *packet, size_t packetSize, char *buffer, size_t bufferSize)
{
    if (!packet || !buffer || bufferSize == 0 || packetSize < avionics_packet::kPacketHeaderSize)
        return false;

    const size_t payloadLength = packet[1];
    if (packetSize != avionics_packet::kPacketHeaderSize + payloadLength || payloadLength + 1U > bufferSize)
        return false;

    for (size_t i = 0; i < payloadLength; ++i)
    {
        const uint8_t byte = packet[avionics_packet::kPacketHeaderSize + i];
        if (!isPrintableAscii(byte))
            return false;
        buffer[i] = static_cast<char>(byte);
    }

    buffer[payloadLength] = '\0';
    return true;
}

void printVideoStatusPayload(const char *payload)
{
    Serial.printf("  status=%s\n", payload + 7);
}

void printVideoAckPayload(char *payload)
{
    char *command = strtok(payload + 4, ":");
    char *result = strtok(nullptr, "");

    Serial.printf("  ack_command=%s\n", command ? command : "<missing>");
    Serial.printf("  ack_result=%s\n", result ? result : "<missing>");
}

void printHeartbeatPayload(char *payload)
{
    char *sequence = strtok(payload + 3, ":");
    char *telemPrefix = strtok(nullptr, ":");
    char *telemState = strtok(nullptr, ":");
    char *telemAge = strtok(nullptr, ":");
    char *telemCount = strtok(nullptr, ":");
    char *videoStatus = strtok(nullptr, "");

    Serial.printf("  heartbeat_seq=%s\n", sequence ? sequence : "<missing>");

    if (telemPrefix && strcmp(telemPrefix, "telem") == 0)
    {
        Serial.printf("  telemetry_state=%s\n", telemState ? telemState : "<missing>");
        Serial.printf("  telemetry_age_s=%s\n", telemAge ? telemAge : "<missing>");
        Serial.printf("  telemetry_count=%s\n", telemCount ? telemCount : "<missing>");
    }
    else
    {
        Serial.println("  telemetry=unparsed");
    }

    Serial.printf("  video_status=%s\n", videoStatus ? videoStatus : "<missing>");
}

void printCommandPayload(const uint8_t *packet, size_t packetSize)
{
    char payload[avionics_packet::kMaxPayloadSize + 1] = {};
    if (!payloadToAscii(packet, packetSize, payload, sizeof(payload)))
    {
        Serial.println("  command payload is non-ASCII or malformed");
        return;
    }

    Serial.printf("  ascii_payload=%s\n", payload);

    if (strncmp(payload, "ack:", 4) == 0)
    {
        printVideoAckPayload(payload);
        return;
    }

    if (strncmp(payload, "status:", 7) == 0)
    {
        printVideoStatusPayload(payload);
        return;
    }

    if (strncmp(payload, "hb:", 3) == 0)
    {
        printHeartbeatPayload(payload);
        return;
    }

    Serial.printf("  command=%s\n", payload);
}

void printHelp()
{
    Serial.println("USB bridge commands:");
    Serial.println("  /help           - show this help");
    Serial.println("  /decode on|off  - enable or disable telemetry decoding");
    Serial.println("  /raw on|off     - mirror raw UART bytes back to USB");
    Serial.println("  /hex AA BB CC   - send raw hex bytes to UART");
    Serial.println("  any other line  - forwarded to UART with newline");
}

void printAviTelemetry(const avionics_packet::AviTelemetry &telemetry)
{
    Serial.println("AVITELEM");
    Serial.printf("  alt_ft=%.1f vel_mps=%.1f accel_mps2=%.1f\n",
                  telemetry.positionZFeet,
                  telemetry.velocityZMs,
                  telemetry.accelZMs2);
    Serial.printf("  baro_agl_ft=%s",
                  telemetry.hasBaroAgl ? "" : "N/A");
    if (telemetry.hasBaroAgl)
        Serial.printf("%.1f", telemetry.baroAglFeet);
    Serial.println();
    Serial.printf("  quat=(%.4f, %.4f, %.4f, %.4f)\n",
                  telemetry.quatW,
                  telemetry.quatX,
                  telemetry.quatY,
                  telemetry.quatZ);
    Serial.printf("  battery_v=%s",
                  telemetry.hasBattery ? "" : "N/A");
    if (telemetry.hasBattery)
        Serial.printf("%.2f", telemetry.batteryVolts);
    Serial.println();
    Serial.printf("  gps=%s",
                  telemetry.hasGps ? "" : "N/A");
    if (telemetry.hasGps)
        Serial.printf("%.7f, %.7f", telemetry.latitudeDeg, telemetry.longitudeDeg);
    Serial.println();
}

void printAbTelemetry(const avionics_packet::AbTelemetry &telemetry)
{
    Serial.println("ABTELEM");
    Serial.printf("  alt_ft=%.1f vel_mps=%.1f accel_mps2=%.1f\n",
                  telemetry.positionZFeet,
                  telemetry.velocityZMs,
                  telemetry.accelZMs2);
    Serial.printf("  baro_agl_ft=%s",
                  telemetry.hasBaroAgl ? "" : "N/A");
    if (telemetry.hasBaroAgl)
        Serial.printf("%.1f", telemetry.baroAglFeet);
    Serial.println();
    Serial.printf("  quat=(%.4f, %.4f, %.4f, %.4f)\n",
                  telemetry.quatW,
                  telemetry.quatX,
                  telemetry.quatY,
                  telemetry.quatZ);
    Serial.printf("  battery_v=%s",
                  telemetry.hasBattery ? "" : "N/A");
    if (telemetry.hasBattery)
        Serial.printf("%.2f", telemetry.batteryVolts);
    Serial.println();
    Serial.printf("  motor_battery_v=%s",
                  telemetry.hasMotorBattery ? "" : "N/A");
    if (telemetry.hasMotorBattery)
        Serial.printf("%.2f", telemetry.motorBatteryVolts);
    Serial.println();
    Serial.printf("  desired_deg=%.1f actual_deg=%.1f predicted_apogee_ft=%.1f\n",
                  telemetry.desiredAngleDeg,
                  telemetry.actualAngleDeg,
                  telemetry.predictedApogeeFeet);
}

void handlePacket(const uint8_t *packet, size_t packetSize)
{
    const uint8_t type = packet[0];
    const uint8_t payloadLength = packet[1];

    Serial.printf("\nPacket: type=%u (%s), payload_len=%u, raw=",
                  type,
                  messageTypeName(type),
                  payloadLength);
    printHexBytes(packet, packetSize);
    Serial.println();

    if (type == static_cast<uint8_t>(avionics_packet::MessageType::AVITELEM))
    {
        avionics_packet::AviTelemetry telemetry;
        if (avionics_packet::decodeAviTelemetry(packet, packetSize, telemetry))
            printAviTelemetry(telemetry);
        else
            Serial.println("  decode failed for AVITELEM");
        return;
    }

    if (type == static_cast<uint8_t>(avionics_packet::MessageType::ABTELEM))
    {
        avionics_packet::AbTelemetry telemetry;
        if (avionics_packet::decodeAbTelemetry(packet, packetSize, telemetry))
            printAbTelemetry(telemetry);
        else
            Serial.println("  decode failed for ABTELEM");
        return;
    }

    if (type == static_cast<uint8_t>(avionics_packet::MessageType::ABCMD) ||
        type == static_cast<uint8_t>(avionics_packet::MessageType::AVICMD) ||
        type == static_cast<uint8_t>(avionics_packet::MessageType::LIVECMD) ||
        type == static_cast<uint8_t>(avionics_packet::MessageType::NOSEVIDCMD) ||
        type == static_cast<uint8_t>(avionics_packet::MessageType::ABVIDCMD))
    {
        printCommandPayload(packet, packetSize);
        return;
    }

    Serial.println("  payload left undecoded");
}

bool parseOnOffValue(const char *value, bool &outValue)
{
    if (value == nullptr)
        return false;

    if (strcmp(value, "on") == 0)
    {
        outValue = true;
        return true;
    }

    if (strcmp(value, "off") == 0)
    {
        outValue = false;
        return true;
    }

    return false;
}

bool tryParseHexByte(const char *token, uint8_t &value)
{
    if (token == nullptr || *token == '\0')
        return false;

    char *end = nullptr;
    const long parsed = strtol(token, &end, 16);
    if (end == token || *end != '\0' || parsed < 0L || parsed > 255L)
        return false;

    value = static_cast<uint8_t>(parsed);
    return true;
}

void sendHexBytesToUart(char *args)
{
    if (args == nullptr || *args == '\0')
    {
        Serial.println("Usage: /hex AA BB CC");
        return;
    }

    uint8_t bytes[avionics_packet::kMaxPacketSize] = {};
    size_t count = 0;

    char *token = strtok(args, " ,");
    while (token != nullptr)
    {
        if (count >= sizeof(bytes))
        {
            Serial.println("Too many hex bytes for one send");
            return;
        }

        uint8_t value = 0;
        if (!tryParseHexByte(token, value))
        {
            Serial.printf("Invalid hex byte: %s\n", token);
            return;
        }

        bytes[count++] = value;
        token = strtok(nullptr, " ,");
    }

    if (count == 0)
    {
        Serial.println("No bytes parsed");
        return;
    }

    kTelemetryPort.write(bytes, count);
    Serial.print("Sent raw hex to UART: ");
    printHexBytes(bytes, count);
    Serial.println();
}

void processUsbCommand(char *line)
{
    if (line[0] == '\0')
        return;

    if (line[0] != '/')
    {
        kTelemetryPort.write(reinterpret_cast<const uint8_t *>(line), strlen(line));
        kTelemetryPort.write('\n');
        Serial.print("UART <= ");
        Serial.println(line);
        return;
    }

    if (strcmp(line, "/help") == 0)
    {
        printHelp();
        return;
    }

    if (strncmp(line, "/decode ", 8) == 0)
    {
        bool enabled = false;
        if (!parseOnOffValue(line + 8, enabled))
        {
            Serial.println("Usage: /decode on|off");
            return;
        }

        decodePacketsEnabled = enabled;
        Serial.printf("Telemetry decode %s\n", enabled ? "enabled" : "disabled");
        if (enabled)
            resetParser();
        return;
    }

    if (strncmp(line, "/raw ", 5) == 0)
    {
        bool enabled = false;
        if (!parseOnOffValue(line + 5, enabled))
        {
            Serial.println("Usage: /raw on|off");
            return;
        }

        rawMirrorEnabled = enabled;
        Serial.printf("Raw UART mirror %s\n", enabled ? "enabled" : "disabled");
        return;
    }

    if (strncmp(line, "/hex ", 5) == 0)
    {
        sendHexBytesToUart(line + 5);
        return;
    }

    Serial.println("Unknown local command. Type /help");
}

void serviceUsbBridge()
{
    while (Serial.available() > 0)
    {
        const int incoming = Serial.read();
        if (incoming < 0)
            continue;

        const char ch = static_cast<char>(incoming);

        if (ch == '\r')
            continue;

        if (ch == '\n')
        {
            usbCommandBuffer[usbCommandLength] = '\0';
            processUsbCommand(usbCommandBuffer);
            usbCommandLength = 0;
            usbCommandBuffer[0] = '\0';
            continue;
        }

        if (usbCommandLength + 1U >= sizeof(usbCommandBuffer))
        {
            Serial.println("USB command too long, clearing buffer");
            usbCommandLength = 0;
            usbCommandBuffer[0] = '\0';
            continue;
        }

        usbCommandBuffer[usbCommandLength++] = ch;
    }
}

void beginPacket(uint8_t type)
{
    parser.packet[0] = type;
    parser.state = RxState::WaitingForLength;
    parser.lastByteAge = 0;
}

void consumeByte(uint8_t byte)
{
    parser.lastByteAge = 0;

    switch (parser.state)
    {
    case RxState::WaitingForType:
        if (avionics_packet::isKnownMessageType(byte))
            beginPacket(byte);
        break;

    case RxState::WaitingForLength:
    {
        parser.packet[1] = byte;
        parser.expectedPayloadLength = byte;
        parser.payloadBytesRead = 0;

        if (byte > avionics_packet::kMaxPayloadSize)
        {
            Serial.printf("Dropped packet: invalid payload length %u for type %u\n",
                          byte,
                          parser.packet[0]);
            resetParser();
            return;
        }

        const size_t fixedPayloadSize =
            avionics_packet::fixedPayloadSize(static_cast<avionics_packet::MessageType>(parser.packet[0]));
        if (fixedPayloadSize != 0 && byte != fixedPayloadSize)
        {
            Serial.printf("Dropped packet: type %u expected payload length %u but got %u\n",
                          parser.packet[0],
                          static_cast<unsigned>(fixedPayloadSize),
                          byte);
            resetParser();
            return;
        }

        if (byte == 0)
        {
            handlePacket(parser.packet, avionics_packet::kPacketHeaderSize);
            resetParser();
            return;
        }

        parser.state = RxState::WaitingForPayload;
        break;
    }

    case RxState::WaitingForPayload:
        parser.packet[avionics_packet::kPacketHeaderSize + parser.payloadBytesRead] = byte;
        ++parser.payloadBytesRead;

        if (parser.payloadBytesRead >= parser.expectedPayloadLength)
        {
            handlePacket(parser.packet,
                         avionics_packet::kPacketHeaderSize + parser.expectedPayloadLength);
            resetParser();
        }
        break;
    }
}

void serviceTelemetryPort()
{
    while (kTelemetryPort.available() > 0)
    {
        const int incoming = kTelemetryPort.read();
        if (incoming < 0)
            continue;

        const uint8_t byte = static_cast<uint8_t>(incoming);

        if (rawMirrorEnabled)
        {
            if (byte == '\n' || byte == '\r' || isPrintableAscii(byte))
                Serial.write(byte);
            else
                Serial.printf("<%02X>", byte);
        }

        if (decodePacketsEnabled)
            consumeByte(byte);
    }

    if (decodePacketsEnabled &&
        parser.state != RxState::WaitingForType &&
        parser.lastByteAge > kPacketTimeoutMs)
    {
        Serial.println("Dropped packet: timeout while receiving payload");
        resetParser();
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

    kTelemetryPort.begin(kTelemetryBaud);
    resetParser();
    heartbeatAge = 0;

    Serial.println("Teensy telemetry sniffer ready");
    Serial.println("Listening on Serial1 at 115200 baud");
    Serial.println("Packet format: [type][length][payload...]");
    Serial.println("Known packet types: 0=ABTELEM, 1=AVITELEM, 2=ABCMD, 3=AVICMD, 4=LIVECMD, 5=NOSEVIDCMD, 6=ABVIDCMD");
    printHelp();
}

void loop()
{
    serviceUsbBridge();
    serviceTelemetryPort();

    if (heartbeatAge > 5000)
    {
        heartbeatAge = 0;
        Serial.println("Waiting for telemetry...");
    }
}
