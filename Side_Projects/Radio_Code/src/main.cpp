#include <Arduino.h>

#ifdef STM32

#define STATUS_LED PB12
#define RADIO_NRST PA6
#define RADIO_BUSY PA7
#define RADIO_NCS PA15
#define RADIO_IO8 PA3
// irq pin
#define RADIO_IO9 PA2

#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3

#include "AvionicsPacketProtocol.h"
#include "Type_2GT.h"
// create the SPI class to input into the radio
SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

namespace
{
constexpr uint32_t kTelemetryBaud = 115200;
constexpr uint32_t kTransmitWindowMs = 750;
constexpr uint32_t kListenWindowMs = 250;
constexpr uint8_t kTransmitRetries = 4;
constexpr uint32_t kTransmitRetryDelayMs = 15;

enum class LinkPhase : uint8_t
{
  Transmit,
  Listen,
};

struct PendingPacket
{
  uint8_t data[avionics_packet::kMaxPacketSize] = {};
  size_t size = 0;
  bool pending = false;
};

struct SerialPacketReader
{
  uint8_t buffer[avionics_packet::kMaxPacketSize] = {};
  size_t size = 0;
  size_t expectedSize = 0;

  void reset()
  {
    size = 0;
    expectedSize = 0;
  }
};

PendingPacket pendingAbTelem;
PendingPacket pendingAviTelem;
SerialPacketReader serialReader;
LinkPhase currentPhase = LinkPhase::Transmit;
uint32_t phaseStartedMs = 0;
bool abSentThisWindow = false;
bool aviSentThisWindow = false;

bool isSupportedTelemetryType(uint8_t rawType)
{
  return rawType == static_cast<uint8_t>(avionics_packet::MessageType::ABTELEM) ||
         rawType == static_cast<uint8_t>(avionics_packet::MessageType::AVITELEM);
}

void queuePendingPacket(PendingPacket &slot, const uint8_t *packet, size_t packetSize)
{
  memcpy(slot.data, packet, packetSize);
  slot.size = packetSize;
  slot.pending = true;
}

void enterTransmitWindow()
{
  currentPhase = LinkPhase::Transmit;
  phaseStartedMs = millis();
  abSentThisWindow = false;
  aviSentThisWindow = false;
}

void enterListenWindow()
{
  currentPhase = LinkPhase::Listen;
  phaseStartedMs = millis();

  const int rc = radio.recieve();
  if (rc != RADIOLIB_ERR_NONE)
  {
    Serial.printf("RAD/Error: failed to enter RX, Error code %d\n", rc);
  }
}

bool transmitPacket(PendingPacket &slot, const char *label)
{
  if (!slot.pending || slot.size == 0)
  {
    return false;
  }

  digitalWrite(STATUS_LED, HIGH);

  int txStatus = RADIOLIB_ERR_NONE;
  bool sent = false;
  for (uint8_t attempt = 0; attempt < kTransmitRetries && !sent; ++attempt)
  {
    txStatus = radio.transmit(slot.data, slot.size);
    if (txStatus == RADIOLIB_ERR_NONE)
    {
      sent = true;
    }
    else
    {
      Serial.printf("RAD/Warn: %s transmit retry attempt=%u err=%d\n",
                    label,
                    static_cast<unsigned>(attempt + 1),
                    txStatus);
      delay(kTransmitRetryDelayMs);
    }
  }

  digitalWrite(STATUS_LED, LOW);

  if (!sent)
  {
    Serial.printf("RAD/Error: %s transmit failed, Error code %d\n", label, txStatus);
    return false;
  }

  slot.pending = false;
  slot.size = 0;
  return true;
}

void processIncomingSerialPacket(const uint8_t *packet, size_t packetSize)
{
  if (!avionics_packet::packetLengthLooksValid(packet, packetSize))
  {
    Serial.println("RAD/Warn: dropped malformed serial packet");
    return;
  }

  switch (static_cast<avionics_packet::MessageType>(packet[0]))
  {
  case avionics_packet::MessageType::ABTELEM:
    queuePendingPacket(pendingAbTelem, packet, packetSize);
    break;
  case avionics_packet::MessageType::AVITELEM:
    queuePendingPacket(pendingAviTelem, packet, packetSize);
    break;
  default:
    Serial.printf("RAD/Warn: unsupported uplink packet type=%u\n", static_cast<unsigned>(packet[0]));
    break;
  }
}

void consumeSerialByte(uint8_t byteValue)
{
  if (serialReader.size == 0)
  {
    if (!isSupportedTelemetryType(byteValue))
    {
      Serial.printf("RAD/Warn: dropped unexpected serial type=%u\n", static_cast<unsigned>(byteValue));
      return;
    }
  }

  if (serialReader.size >= avionics_packet::kMaxPacketSize)
  {
    serialReader.reset();
  }

  serialReader.buffer[serialReader.size++] = byteValue;

  if (serialReader.size == avionics_packet::kPacketHeaderSize)
  {
    serialReader.expectedSize = avionics_packet::kPacketHeaderSize + serialReader.buffer[1];
    if (serialReader.expectedSize > avionics_packet::kMaxPacketSize)
    {
      Serial.printf("RAD/Warn: dropped oversized serial payload len=%u\n",
                    static_cast<unsigned>(serialReader.buffer[1]));
      serialReader.reset();
    }
  }

  if (serialReader.expectedSize != 0 && serialReader.size == serialReader.expectedSize)
  {
    processIncomingSerialPacket(serialReader.buffer, serialReader.size);
    serialReader.reset();
  }
}

void pollSerialInput()
{
  while (Serial.available())
  {
    const int in = Serial.read();
    if (in < 0)
    {
      break;
    }

    consumeSerialByte(static_cast<uint8_t>(in));
  }
}

void forwardRadioPacketToSerial()
{
  if (!radio.hasData())
  {
    return;
  }

  const size_t packetSize = radio.getPacketLength();
  if (packetSize == 0 || packetSize > avionics_packet::kMaxPacketSize)
  {
    Serial.printf("RAD/Warn: dropped radio packet with invalid size=%u\n",
                  static_cast<unsigned>(packetSize));
    uint8_t scratch[avionics_packet::kMaxPacketSize] = {};
    radio.readData(scratch, avionics_packet::kMaxPacketSize);
    radio.recieve();
    return;
  }

  uint8_t packet[avionics_packet::kMaxPacketSize] = {};
  const int rc = radio.readData(packet, packetSize);
  if (rc != RADIOLIB_ERR_NONE)
  {
    Serial.printf("RAD/Warn: radio read failed, Error code %d\n", rc);
    radio.recieve();
    return;
  }

  if (!avionics_packet::packetLengthLooksValid(packet, packetSize))
  {
    Serial.println("RAD/Warn: dropped malformed radio packet");
    radio.recieve();
    return;
  }

  const size_t written = Serial.write(packet, packetSize);
  if (written != packetSize)
  {
    Serial.printf("RAD/Warn: serial forward short write=%u expected=%u\n",
                  static_cast<unsigned>(written),
                  static_cast<unsigned>(packetSize));
  }

  radio.recieve();
}

void serviceTransmitWindow()
{
  if (!abSentThisWindow && pendingAbTelem.pending)
  {
    if (transmitPacket(pendingAbTelem, "ABTELEM"))
    {
      abSentThisWindow = true;
    }
  }

  const bool canSendAvi = !aviSentThisWindow && pendingAviTelem.pending && (!pendingAbTelem.pending || abSentThisWindow);
  if (canSendAvi)
  {
    if (transmitPacket(pendingAviTelem, "AVITELEM"))
    {
      aviSentThisWindow = true;
    }
  }

  const uint32_t elapsedMs = millis() - phaseStartedMs;
  const bool sentFullTelemetryPair = abSentThisWindow && aviSentThisWindow;
  if (sentFullTelemetryPair || elapsedMs >= kTransmitWindowMs)
  {
    enterListenWindow();
  }
}

void serviceListenWindow()
{
  forwardRadioPacketToSerial();

  const uint32_t elapsedMs = millis() - phaseStartedMs;
  if (elapsedMs >= kListenWindowMs)
  {
    enterTransmitWindow();
  }
}
} // namespace

void radInt(void)
{
  radio.respondToIrq();
  digitalWrite(STATUS_LED, LOW);
}

void setup()
{

  // Serial1 (USART) pins
  Serial.setRx(PB7_ALT1); // alt pin defs
  Serial.setTx(PB6_ALT2);
  Serial.setTimeout(5000);
  Serial.begin(kTelemetryBaud);

  pinMode(STATUS_LED, OUTPUT);
  int radio_init_statuscode = radio.begin();
  if (radio_init_statuscode != RADIOLIB_ERR_NONE)
  {
    Serial.printf("RAD/Error: Radio not initialized, Error code %d\n", radio_init_statuscode);
  }
  else
  {
    Serial.println("RAD/Info: Radio Initialized");
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
  }

  radio.onIrq(radInt);
  enterTransmitWindow();
}

void loop()
{
  pollSerialInput();

  if (currentPhase == LinkPhase::Transmit)
  {
    serviceTransmitWindow();
  }
  else
  {
    serviceListenWindow();
  }
}
#endif
/*
  Dont worry about this code, it is just a USB to serial bridge

*/
#ifdef TEENSY
namespace
{
constexpr unsigned long TELEMETRY_INTERVAL_MS = 500;

void buildTelemetryPacket(char *packet, size_t packetSize)
{
  const unsigned long nowMs = millis();
  const unsigned long timeSeconds = nowMs / 1000UL;
  const long phase = (long)((nowMs / 500UL) % 80UL);

  // Simple synthetic flight profile for bench testing the serial link.
  const long positionMeters = 1200L + (phase * 18L);
  const long velocityMetersPerSecond = 45L + ((phase % 12L) - 6L);
  const long pressureDeciKpa = 1013L - (phase * 2L);
  const long temperatureC = 24L - (phase / 20L);

  snprintf(packet,
           packetSize,
           "TELEM/%ld,%ld,%ld,%lu,%ld",
           positionMeters,
           velocityMetersPerSecond,
           pressureDeciKpa,
           timeSeconds,
           temperatureC);
}
} // namespace

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop()
{
  static unsigned long lastTelemetryMs = 0;

  while (Serial1.available())
  {
    Serial.write((char)Serial1.read());
  }

  while (Serial.available())
  {
    Serial1.write((char)Serial.read());
  }

  const unsigned long nowMs = millis();
  if ((nowMs - lastTelemetryMs) >= TELEMETRY_INTERVAL_MS)
  {
    lastTelemetryMs = nowMs;

    char telemetryPacket[50];
    buildTelemetryPacket(telemetryPacket, sizeof(telemetryPacket));
    Serial1.println(telemetryPacket);
    Serial.printf("Sent telemetry: %s\n", telemetryPacket);
  }
}

#endif
