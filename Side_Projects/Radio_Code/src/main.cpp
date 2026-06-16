#include <Arduino.h>

#ifdef STM32

// ---------------------------------------------------------------------------
// ARC flight radio (rocket side) -- LR1121 via Type2GT.
//
// This radio is an ARC node (address RADIO_CMD = 0x20) and the half-duplex
// link MASTER. Bare-bones first-light behaviour:
//   * every kCyclePeriodMs it transmits one downlink ARC frame -- a heartbeat
//     placeholder, or a frame queued from the host UART -- then listens, so a
//     command window opens right after each downlink and the ground can talk
//     back without colliding;
//   * frames addressed to us (dst == 0x20) are handled locally. Today that is
//     RADIO SET_FREQUENCY / SET_PHY_PROFILE: ACK on the old settings, switch
//     after a grace delay, and revert to defaults if we then hear nothing.
//     Everything else is forwarded to the host UART, COBS-framed.
//
// On-air framing: a raw ARC frame is the LoRa payload (the LR1121 already
// delimits and CRCs packets, so no COBS over the air). On the host UART we use
// COBS, matching the rest of the ARC serial links.
//
// TODO(first-light+):
//   * replace the self-generated heartbeat with real telemetry from the Teensy
//     hub / FC over the host UART;
//   * upgrade the "listen after every packet" MAC to the full 500 ms superframe
//     once high-rate telemetry exists;
//   * keep RADIO_HOST_UART_DEBUG disabled when connected to the hub. The host
//     UART carries COBS-framed ARC frames in integration mode.
// ---------------------------------------------------------------------------

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

#include "Type_2GT.h"

// arc-protocol C library (pulled from github.com/DrewBrandt/arc-protocol).
// Headers carry their own extern "C" guards.
#include "arc_protocol.h"
#include "arc_messages_netmgmt.h"
#include "arc_messages_radio.h"

#ifndef RADIO_HOST_UART_DEBUG
#define RADIO_HOST_UART_DEBUG 0
#endif

#if RADIO_HOST_UART_DEBUG
#define RAD_LOG_PRINT(...)   do { Serial.print(__VA_ARGS__); } while (0)
#define RAD_LOG_PRINTLN(...) do { Serial.println(__VA_ARGS__); } while (0)
#define RAD_LOG_PRINTF(...)  do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define RAD_LOG_PRINT(...)   do { } while (0)
#define RAD_LOG_PRINTLN(...) do { } while (0)
#define RAD_LOG_PRINTF(...)  do { } while (0)
#endif

SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

namespace
{
// --- ARC identity / peer ---
constexpr uint8_t kMyAddr = ARC_ADDR_RADIO_CMD;   // 0x20 (rocket command/status radio)
constexpr uint8_t kGroundAddr = ARC_ADDR_GROUND;  // 0x01 (ground station / GS app)

// --- link timing ---
constexpr uint32_t kHostBaud = 115200;
constexpr uint32_t kCyclePeriodMs = 1000;  // downlink cadence; the gaps are the command window
constexpr uint32_t kHostHeartbeatMs = 5000;
constexpr uint32_t kHostDebugMs = 1000;   // TEMP bench UART diagnostics

// --- frequency control ---
constexpr float kDefaultFreqMHz = 915.0f;
constexpr uint32_t kFreqSwitchDelayMs = 1000;  // grace so the ACK reaches ground before we hop
constexpr uint32_t kRevertTimeoutMs = 5000;    // no valid RX after a hop -> revert to default
constexpr uint8_t kDefaultPhyProfile = ARC_RADIO_PHY_PROFILE_SAFE_BW125;
constexpr int8_t kDefaultTxPowerDbm = 22;      // reported in STATUS_REPORT; driver has no power readback yet

// --- per-boot ARC state ---
uint8_t g_session = 1;
uint16_t g_seq = 0;
uint32_t g_lastCycleMs = 0;
uint32_t g_lastRxMs = 0;

// --- frequency state ---
float g_curFreqMHz = kDefaultFreqMHz;
bool g_freqSwitchPending = false;
uint32_t g_freqSwitchAtMs = 0;
float g_freqSwitchTargetMHz = kDefaultFreqMHz;
uint8_t g_curPhyProfile = kDefaultPhyProfile;
bool g_phySwitchPending = false;
uint32_t g_phySwitchAtMs = 0;
uint8_t g_phySwitchTarget = kDefaultPhyProfile;

// --- host UART COBS reassembly + one downlink slot ---
uint8_t g_hostRx[ARC_MAX_ENCODED_SIZE + 4];
size_t g_hostRxLen = 0;
uint8_t g_dlFrame[ARC_MAX_FRAME_SIZE];
size_t g_dlLen = 0;
bool g_dlPending = false;
uint32_t g_lastHostDebugMs = 0;
uint32_t g_hostByteCount = 0;
uint32_t g_hostFrameCount = 0;
uint32_t g_loraTxCount = 0;
uint32_t g_loraRxCount = 0;
uint32_t g_lastHostHeartbeatMs = 0;
float g_lastRssi = 0.0f;  // from the most recent valid LoRa RX, for STATUS_REPORT
float g_lastSnr = 0.0f;

void sendFrameOverLora(const uint8_t *frame, size_t n)
{
  digitalWrite(STATUS_LED, HIGH);
  const int rc = radio.transmit(frame, n);  // blocking
  digitalWrite(STATUS_LED, LOW);
  if (rc != RADIOLIB_ERR_NONE)
  {
    RAD_LOG_PRINTF("RAD/Warn: LoRa TX err=%d\n", rc);
    return;
  }
  g_loraTxCount++;
}

void writeFrameToHostCobs(const uint8_t *frame, size_t n)
{
  uint8_t enc[ARC_MAX_ENCODED_SIZE];
  const int m = arc_cobs_encode(frame, n, enc, sizeof(enc));
  if (m < 0)
  {
    return;
  }
  Serial.write(enc, m);  // enc already ends in the 0x00 delimiter
}

void sendHeartbeat()
{
  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), kMyAddr, kGroundAddr,
                                0, g_session, g_seq++,
                                ARC_FAMILY_NETMGMT, ARC_NETMGMT_HEARTBEAT,
                                nullptr, 0);
  if (n > 0)
  {
    sendFrameOverLora(frame, static_cast<size_t>(n));
  }
}

void sendHostHeartbeat()
{
  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), kMyAddr, ARC_ADDR_BROADCAST,
                                0, g_session, g_seq++,
                                ARC_FAMILY_NETMGMT, ARC_NETMGMT_HEARTBEAT,
                                nullptr, 0);
  if (n > 0)
  {
    writeFrameToHostCobs(frame, static_cast<size_t>(n));
  }
}

void sendAck(uint8_t to, uint16_t ackedSeq)
{
  const arc_netmgmt_ack_t ack = {ackedSeq};
  uint8_t payload[ARC_NETMGMT_ACK_PAYLOAD_SIZE];
  if (arc_netmgmt_ack_encode(&ack, payload, sizeof(payload)) < 0)
  {
    return;
  }
  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), kMyAddr, to,
                                ARC_FLAG_ACK, g_session, g_seq++,
                                ARC_FAMILY_NETMGMT, ARC_NETMGMT_ACK,
                                payload, sizeof(payload));
  if (n > 0)
  {
    sendFrameOverLora(frame, static_cast<size_t>(n));
  }
}

void sendStatusReport(uint8_t to)
{
  arc_radio_status_report_t rep;
  rep.frequency_hz = static_cast<uint32_t>(g_curFreqMHz * 1.0e6f);
  rep.tx_power_dbm = kDefaultTxPowerDbm;  // no power readback in the driver yet
  rep.rssi_dbm = static_cast<int8_t>(g_lastRssi);
  rep.snr_db = static_cast<int8_t>(g_lastSnr);
  rep.error_flags = 0;
  rep.packets_rx = static_cast<uint16_t>(g_loraRxCount);
  rep.packets_tx = static_cast<uint16_t>(g_loraTxCount);

  uint8_t payload[ARC_RADIO_STATUS_REPORT_PAYLOAD_SIZE];
  if (arc_radio_status_report_encode(&rep, payload, sizeof(payload)) < 0)
  {
    return;
  }
  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), kMyAddr, to,
                                0, g_session, g_seq++,
                                ARC_FAMILY_RADIO, ARC_RADIO_STATUS_REPORT,
                                payload, sizeof(payload));
  if (n > 0)
  {
    sendFrameOverLora(frame, static_cast<size_t>(n));
    radio.recieve();  // reopen the listen window after the reply
  }
}

void handleSetFrequency(const arc_frame_t *f)
{
  arc_radio_set_frequency_t msg;
  if (arc_radio_set_frequency_decode(f->payload, f->payload_len, &msg) != ARC_OK)
  {
    RAD_LOG_PRINTLN("RAD/Warn: bad SET_FREQUENCY payload");
    return;
  }
  const float targetMHz = static_cast<float>(msg.frequency_hz) / 1.0e6f;
  RAD_LOG_PRINTF("RAD/Info: SET_FREQUENCY -> %.3f MHz (hop in %lu ms)\n",
                 targetMHz, static_cast<unsigned long>(kFreqSwitchDelayMs));

  g_freqSwitchTargetMHz = targetMHz;
  g_freqSwitchAtMs = millis() + kFreqSwitchDelayMs;
  g_freqSwitchPending = true;
}

void handleSetPhyProfile(const arc_frame_t *f)
{
  arc_radio_set_phy_profile_t msg;
  if (arc_radio_set_phy_profile_decode(f->payload, f->payload_len, &msg) != ARC_OK)
  {
    RAD_LOG_PRINTLN("RAD/Warn: bad SET_PHY_PROFILE payload");
    return;
  }
  if (msg.profile_id != ARC_RADIO_PHY_PROFILE_SAFE_BW125 &&
      msg.profile_id != ARC_RADIO_PHY_PROFILE_FAST_BW500)
  {
    RAD_LOG_PRINTF("RAD/Warn: unknown PHY profile %u\n", msg.profile_id);
    return;
  }

  RAD_LOG_PRINTF("RAD/Info: SET_PHY_PROFILE -> %u (switch in %lu ms)\n",
                 msg.profile_id, static_cast<unsigned long>(kFreqSwitchDelayMs));

  g_phySwitchTarget = msg.profile_id;
  g_phySwitchAtMs = millis() + kFreqSwitchDelayMs;
  g_phySwitchPending = true;
}

void applyFreqSwitch()
{
  const int rc = radio.setFrequency(g_freqSwitchTargetMHz);
  if (rc == RADIOLIB_ERR_NONE)
  {
    g_curFreqMHz = g_freqSwitchTargetMHz;
    RAD_LOG_PRINTF("RAD/Info: hopped to %.3f MHz\n", g_curFreqMHz);
  }
  else
  {
    RAD_LOG_PRINTF("RAD/Error: setFrequency err=%d\n", rc);
  }
  g_freqSwitchPending = false;
  g_lastRxMs = millis();  // grace before the revert check arms
  radio.recieve();
}

void applyPhySwitch()
{
  const int rc = radio.applyPhyProfile(g_phySwitchTarget);
  if (rc == RADIOLIB_ERR_NONE)
  {
    g_curPhyProfile = g_phySwitchTarget;
    RAD_LOG_PRINTF("RAD/Info: PHY profile -> %u\n", g_curPhyProfile);
  }
  else
  {
    RAD_LOG_PRINTF("RAD/Error: applyPhyProfile err=%d\n", rc);
  }
  g_phySwitchPending = false;
  g_lastRxMs = millis();
  radio.recieve();
}

void maybeRevertLinkSettings()
{
  if (g_curFreqMHz == kDefaultFreqMHz && g_curPhyProfile == kDefaultPhyProfile)
  {
    return;
  }
  if (millis() - g_lastRxMs < kRevertTimeoutMs)
  {
    return;
  }
  RAD_LOG_PRINTF("RAD/Warn: silent %lu ms, reverting link settings freq %.3f -> %.3f MHz profile %u -> %u\n",
                 static_cast<unsigned long>(kRevertTimeoutMs), g_curFreqMHz, kDefaultFreqMHz,
                 g_curPhyProfile, kDefaultPhyProfile);
  radio.setFrequency(kDefaultFreqMHz);
  radio.applyPhyProfile(kDefaultPhyProfile);
  g_curFreqMHz = kDefaultFreqMHz;
  g_curPhyProfile = kDefaultPhyProfile;
  g_lastRxMs = millis();
  radio.recieve();
}

void serviceRx()
{
  if (!radio.hasData())
  {
    return;
  }
  const size_t n = radio.getPacketLength();
  if (n == 0 || n > ARC_MAX_FRAME_SIZE)
  {
    RAD_LOG_PRINTF("RAD/Warn: LoRa packet len=%u ignored\n", static_cast<unsigned>(n));
    uint8_t scratch[ARC_MAX_FRAME_SIZE];
    radio.readData(scratch, sizeof(scratch));
    radio.recieve();
    return;
  }
  uint8_t raw[ARC_MAX_FRAME_SIZE];
  const int rc = radio.readData(raw, n);
  radio.recieve();
  if (rc != RADIOLIB_ERR_NONE)
  {
    RAD_LOG_PRINTF("RAD/Warn: LoRa read err=%d\n", rc);
    return;
  }
  if (RADIO_HOST_UART_DEBUG)
  {
    RAD_LOG_PRINTF("RAD/DBG rx %u B rssi=%.1f snr=%.1f hex=",
                   static_cast<unsigned>(n), radio.getRSSI(), radio.getSNR());
    const size_t shown = n < 24 ? n : 24;
    for (size_t i = 0; i < shown; i++)
    {
      RAD_LOG_PRINTF("%02X", raw[i]);
    }
    if (shown < n)
    {
      RAD_LOG_PRINT("...");
    }
    RAD_LOG_PRINTLN();
  }

  arc_frame_t f;
  if (arc_frame_parse(raw, n, &f) != ARC_OK)
  {
    RAD_LOG_PRINTLN("RAD/Warn: bad ARC frame from LoRa");
    return;
  }
  g_lastRxMs = millis();
  g_loraRxCount++;
  g_lastRssi = radio.getRSSI();
  g_lastSnr = radio.getSNR();

  const bool addressedToMe = (f.dst == kMyAddr);
  if (addressedToMe && (f.flags & ARC_FLAG_RELIABLE))
  {
    // Transport-level ACK is common ARC behavior. Send it before any payload
    // side effects so link-setting commands still ACK on the old RF settings.
    sendAck(f.src, f.seq);
  }

  // Self-addressed control we act on locally (recipient parsing).
  if ((addressedToMe || f.dst == ARC_ADDR_BROADCAST) &&
      f.family == ARC_FAMILY_RADIO && f.type == ARC_RADIO_SET_FREQUENCY)
  {
    handleSetFrequency(&f);
    return;  // consumed
  }
  if ((addressedToMe || f.dst == ARC_ADDR_BROADCAST) &&
      f.family == ARC_FAMILY_RADIO && f.type == ARC_RADIO_SET_PHY_PROFILE)
  {
    handleSetPhyProfile(&f);
    return;  // consumed
  }
  if (addressedToMe && f.family == ARC_FAMILY_RADIO && f.type == ARC_RADIO_GET_STATUS)
  {
    sendStatusReport(f.src);
    return;  // consumed
  }

  if (addressedToMe)
  {
    return;  // generic reliable ping/unknown local command was ACKed above
  }

  // Everything else: forward up to the host (FC / Teensy hub) as COBS.
  writeFrameToHostCobs(raw, n);
}

void pollHostSerial()
{
  while (Serial.available())
  {
    const int c = Serial.read();
    if (c < 0)
    {
      break;
    }
    const uint8_t b = static_cast<uint8_t>(c);
    g_hostByteCount++;
    if (b == 0x00)
    {
      if (g_hostRxLen == 0)
      {
        continue;  // delimiter / resync
      }
      if (g_hostRxLen >= sizeof(g_hostRx))
      {
        g_hostRxLen = 0;
        continue;
      }
      g_hostRx[g_hostRxLen++] = 0x00;  // arc_cobs_decode wants the trailing 0x00
      uint8_t dec[ARC_MAX_FRAME_SIZE];
      const int decoded = arc_cobs_decode(g_hostRx, g_hostRxLen, dec, sizeof(dec));
      g_hostRxLen = 0;
      if (decoded > 0 && static_cast<size_t>(decoded) <= sizeof(g_dlFrame))
      {
        memcpy(g_dlFrame, dec, static_cast<size_t>(decoded));
        g_dlLen = static_cast<size_t>(decoded);
        g_dlPending = true;  // sent on the next downlink cycle
        g_hostFrameCount++;
      }
    }
    else if (g_hostRxLen < sizeof(g_hostRx))
    {
      g_hostRx[g_hostRxLen++] = b;
    }
    else
    {
      g_hostRxLen = 0;  // overflow, resync
    }
  }
}

void printHostDebug(uint32_t now)
{
  if (now - g_lastHostDebugMs < kHostDebugMs)
  {
    return;
  }
  g_lastHostDebugMs = now;
  RAD_LOG_PRINTF("RAD/DBG alive ms=%lu host_bytes=%lu host_frames=%lu lora_tx=%lu lora_rx=%lu dl_pending=%u freq=%.3f phy=%u rx_state=%d rx_rc=%d rx_start=%lu\n",
                 static_cast<unsigned long>(now),
                 static_cast<unsigned long>(g_hostByteCount),
                 static_cast<unsigned long>(g_hostFrameCount),
                 static_cast<unsigned long>(g_loraTxCount),
                 static_cast<unsigned long>(g_loraRxCount),
                 g_dlPending ? 1U : 0U,
                 g_curFreqMHz,
                 g_curPhyProfile,
                 radio.getState(),
                 radio.getLastReceiveRc(),
                 static_cast<unsigned long>(radio.getReceiveStartCount()));
}
}  // namespace

void radInt(void)
{
  radio.respondToIrq();
}

void setup()
{
  // Host UART (USART1) -- to the FC / Teensy hub. Also carries debug logs for
  // now (see TODO at top).
  Serial.setRx(PB7_ALT1);
  Serial.setTx(PB6_ALT2);
  Serial.setTimeout(5000);
  Serial.begin(kHostBaud);
  delay(50);
  RAD_LOG_PRINTLN("RAD/DBG host UART up on USART1 PB6/PB7 @115200");

  pinMode(STATUS_LED, OUTPUT);

  // Random session per boot so the ground resets its dedup window.
  randomSeed(micros());
  g_session = static_cast<uint8_t>(random(1, 256));

  RAD_LOG_PRINTLN("RAD/DBG calling radio.begin()");
  const int rc = radio.begin();
  if (rc != RADIOLIB_ERR_NONE)
  {
    RAD_LOG_PRINTF("RAD/Error: radio begin err=%d\n", rc);
  }
  else
  {
    RAD_LOG_PRINTF("RAD/Info: ARC flight radio up (addr 0x%02X, session 0x%02X)\n",
                   kMyAddr, g_session);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
    digitalWrite(STATUS_LED, LOW);
  }

  radio.onIrq(radInt);
  g_curFreqMHz = kDefaultFreqMHz;
  g_curPhyProfile = kDefaultPhyProfile;
  g_lastCycleMs = millis();
  g_lastRxMs = millis();
  const int rxRc = radio.recieve();  // start listening between cycles
  RAD_LOG_PRINTF("RAD/DBG startReceive -> %d\n", rxRc);
  sendHostHeartbeat();
  g_lastHostHeartbeatMs = millis();
}

void loop()
{
  pollHostSerial();
  serviceRx();

  const uint32_t now = millis();
  printHostDebug(now);
  if (now - g_lastHostHeartbeatMs >= kHostHeartbeatMs)
  {
    g_lastHostHeartbeatMs = now;
    sendHostHeartbeat();
  }

  if (g_freqSwitchPending && static_cast<int32_t>(now - g_freqSwitchAtMs) >= 0)
  {
    applyFreqSwitch();
  }
  if (g_phySwitchPending && static_cast<int32_t>(now - g_phySwitchAtMs) >= 0)
  {
    applyPhySwitch();
  }
  maybeRevertLinkSettings();

  if (now - g_lastCycleMs >= kCyclePeriodMs)
  {
    g_lastCycleMs = now;
    if (g_dlPending)
    {
      sendFrameOverLora(g_dlFrame, g_dlLen);
      g_dlPending = false;
    }
    else
    {
      sendHeartbeat();
    }
    radio.recieve();  // reopen the command window
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
