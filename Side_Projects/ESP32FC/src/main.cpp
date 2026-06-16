// ARC ground radio (ground-station side) -- ESP32-S3 + Type2GT (LR1121) + BLE NUS.
//
// Bridges the LoRa link to the GS computer over BLE (Nordic UART Service):
//   * downlink:  LoRa frame (raw ARC) -> COBS -> BLE notify  -> GS app
//   * uplink:    GS app -> BLE write (COBS ARC) -> queued -> LoRa TX
//
// Half-duplex SLAVE: it transmits a queued uplink frame only inside the command
// window the flight radio opens right after each downlink (i.e. immediately
// after we receive one), so the two ends never step on each other.
//
// It also mirrors link-setting changes: when it relays a RADIO SET_FREQUENCY
// or SET_PHY_PROFILE up and then sees the flight radio's ACK come back, it
// schedules its own switch (same grace delay) and reverts to defaults if the
// link goes silent.
//
// Uses the same Type2GT driver as the flight radio, so the PHY/RF-switch config
// is guaranteed identical. On-air = raw ARC frame (LR1121 delimits + CRCs);
// BLE = COBS-framed ARC, matching every other ARC serial link.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_random.h>

#include "Type_2GT.h"
#include "arc_protocol.h"
#include "arc_messages_netmgmt.h"
#include "arc_messages_radio.h"

// ======================= LoRa (Type2GT) ========================
SPIClass spi(HSPI);
// Type2GT(cs, irq, rst, busy, spi) -- ground-board pinout.
Type2GT radio(/*CS*/ 14, /*DIO1/irq*/ 36, /*RST*/ 38, /*BUSY*/ 37, spi);

// ESP32 GPIO ISRs aren't safe for SPI, so the IRQ only sets a flag; the actual
// respondToIrq()/readData() (which touch SPI) run in loop().
volatile bool g_irqFlag = false;
static void IRAM_ATTR onLoraIrq() { g_irqFlag = true; }

// ======================= ARC link state ========================
static constexpr float kDefaultFreqMHz = 915.0f;
static constexpr uint8_t kDefaultPhyProfile = ARC_RADIO_PHY_PROFILE_SAFE_BW125;
static constexpr int8_t kDefaultTxPowerDbm = 22;  // reported in STATUS_REPORT; driver has no power readback yet
static constexpr uint32_t kFreqSwitchDelayMs = 1000;  // match the flight radio's grace
static constexpr uint32_t kRevertTimeoutMs = 5000;
static constexpr uint32_t kUplinkSlotOffsetMs = 650;
static constexpr uint32_t kImmediateUplinkWindowMs = 900;
static constexpr uint32_t kSelfHeartbeatMs = 1000;  // announce ourselves to the GS app over BLE

static float g_curFreqMHz = kDefaultFreqMHz;
static uint8_t g_curPhyProfile = kDefaultPhyProfile;
static uint32_t g_lastRxMs = 0;
static uint32_t g_lastDebugMs = 0;
static uint32_t g_lastPollMs = 0;
static uint32_t g_lastSelfHeartbeatMs = 0;
static uint32_t g_irqCount = 0;
static uint32_t g_pollCount = 0;
static uint32_t g_downCount = 0;
static uint32_t g_notifyBytes = 0;
static uint32_t g_notifyDropBytes = 0;
static uint32_t g_loraTxCount = 0;
static float g_lastRssi = 0.0f;  // from the most recent downlink, for STATUS_REPORT
static float g_lastSnr = 0.0f;
// Our own ARC identity for unsolicited frames we originate (e.g. STATUS_REPORT).
static uint8_t g_session = 1;
static uint16_t g_seq = 0;

// mirrors: armed when we relay a link-setting command up; fired by matching ACK.
static bool g_freqPendingValid = false;
static uint16_t g_freqPendingSeq = 0;
static float g_freqPendingTargetMHz = kDefaultFreqMHz;
static bool g_phyPendingValid = false;
static uint16_t g_phyPendingSeq = 0;
static uint8_t g_phyPendingTarget = kDefaultPhyProfile;
static bool g_hopPending = false;
static uint32_t g_hopAtMs = 0;
static float g_hopTargetMHz = kDefaultFreqMHz;
static bool g_phyHopPending = false;
static uint32_t g_phyHopAtMs = 0;
static uint8_t g_phyHopTarget = kDefaultPhyProfile;

// uplink queue (produced + consumed in loop(), so no locking needed)
struct UpFrame {
  uint8_t data[ARC_MAX_FRAME_SIZE];
  size_t len;
};
static constexpr int kUpQDepth = 6;
static UpFrame g_upQ[kUpQDepth];
static int g_upHead = 0, g_upTail = 0;

static bool upEnqueue(const uint8_t *d, size_t n) {
  const int nh = (g_upHead + 1) % kUpQDepth;
  if (nh == g_upTail) return false;  // full
  memcpy(g_upQ[g_upHead].data, d, n);
  g_upQ[g_upHead].len = n;
  g_upHead = nh;
  return true;
}
static bool upDequeue(UpFrame &out) {
  if (g_upTail == g_upHead) return false;  // empty
  out = g_upQ[g_upTail];
  g_upTail = (g_upTail + 1) % kUpQDepth;
  return true;
}

// The GS broadcasts a heartbeat ~1 Hz, but our uplink window only opens once per
// downlink. If those periodic heartbeats shared the command FIFO they would fill
// it and starve/drop real commands. Instead we keep only the *latest* heartbeat
// in a single overwrite slot and send it only when no command is queued, so
// liveness still reaches the rocket without ever crowding out commands.
static UpFrame g_hbPending;
static bool g_hbPendingValid = false;

static void hbStore(const uint8_t *d, size_t n) {
  memcpy(g_hbPending.data, d, n);
  g_hbPending.len = n;
  g_hbPendingValid = true;
}

// byte ring: BLE write task -> loop(). Guarded by a portMUX (cross-task).
static constexpr size_t kBleRingSize = 1024;
static uint8_t g_bleRing[kBleRingSize];
static volatile size_t g_bleHead = 0, g_bleTail = 0;
static portMUX_TYPE g_bleMux = portMUX_INITIALIZER_UNLOCKED;

static void bleRingPush(const uint8_t *d, size_t n) {
  portENTER_CRITICAL(&g_bleMux);
  for (size_t i = 0; i < n; i++) {
    const size_t nh = (g_bleHead + 1) % kBleRingSize;
    if (nh == g_bleTail) break;  // full, drop rest
    g_bleRing[g_bleHead] = d[i];
    g_bleHead = nh;
  }
  portEXIT_CRITICAL(&g_bleMux);
}
static int bleRingPop() {
  int r = -1;
  portENTER_CRITICAL(&g_bleMux);
  if (g_bleTail != g_bleHead) {
    r = g_bleRing[g_bleTail];
    g_bleTail = (g_bleTail + 1) % kBleRingSize;
  }
  portEXIT_CRITICAL(&g_bleMux);
  return r;
}

// ======================= BLE (NimBLE NUS) ======================
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";  // Write
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";  // Notify

NimBLEServer *g_bleServer = nullptr;
NimBLECharacteristic *g_txChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;
static constexpr size_t kBleNotifyChunkBytes = 240;

static void ble_start_advertising(const char *reason) {
  const bool ok = g_bleServer ? g_bleServer->startAdvertising() : NimBLEDevice::startAdvertising();
  USBSerial.printf("[BLE] advertising %s -> %s\n", reason, ok ? "ok" : "failed");
}

static void ble_notify(const uint8_t *data, size_t len) {
  if (!g_hasClient || !g_notifyEnabled || !g_txChar || len == 0) {
    g_notifyDropBytes += len;
    return;
  }
  g_txChar->setValue((uint8_t *)data, len);
  g_txChar->notify();
  g_notifyBytes += len;
}

static void ble_notify_chunked(const uint8_t *data, size_t len) {
  while (len > 0) {
    const size_t chunk = len > kBleNotifyChunkBytes ? kBleNotifyChunkBytes : len;
    ble_notify(data, chunk);
    data += chunk;
    len -= chunk;
  }
}

static void notifyArcFrameToGs(const uint8_t *frame, size_t n) {
  uint8_t enc[ARC_MAX_ENCODED_SIZE];
  const int m = arc_cobs_encode(frame, n, enc, sizeof(enc));
  if (m > 0) ble_notify_chunked(enc, (size_t)m);
}

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override {
    // Push raw bytes to the ring; loop() does COBS reassembly + ARC parsing.
    std::string v = c->getValue();
    bleRingPush(reinterpret_cast<const uint8_t *>(v.data()), v.size());
  }
};

class TxCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override {
    g_notifyEnabled = (subValue & 0x0001);
    USBSerial.printf("[BLE] TX subscribe -> notify=%d\n", (int)g_notifyEnabled);
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *, NimBLEConnInfo &) override {
    g_hasClient = true;
    USBSerial.println("[BLE] client connected");
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override {
    g_hasClient = false;
    g_notifyEnabled = false;
    ble_start_advertising("restart after disconnect");
  }
};

// ======================= ARC handling ==========================
static void loraTransmit(const uint8_t *frame, size_t n) {
  g_irqFlag = false;  // discard any stale RX/TX edge before switching modes
  const int rc = radio.transmit(frame, n);  // blocking; switches to TX
  if (rc != RADIOLIB_ERR_NONE) USBSerial.printf("[LoRa] uplink TX err=%d\n", rc);
  else {
    g_loraTxCount++;
    USBSerial.printf("[LoRa] uplink %u B sent hex=", (unsigned)n);
    const size_t shown = n < 24 ? n : 24;
    for (size_t i = 0; i < shown; i++) USBSerial.printf("%02X", frame[i]);
    if (shown < n) USBSerial.print("...");
    USBSerial.println();
  }
  g_irqFlag = false;  // blocking TX may leave TX_DONE asserted for loop()
}

static void ackLocalReliableToGs(const arc_frame_t &f) {
  const arc_netmgmt_ack_t ack = {f.seq};
  uint8_t payload[ARC_NETMGMT_ACK_PAYLOAD_SIZE];
  if (arc_netmgmt_ack_encode(&ack, payload, sizeof(payload)) < 0) return;

  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), ARC_ADDR_RADIO_G, f.src,
                                ARC_FLAG_ACK, f.session, f.seq,
                                ARC_FAMILY_NETMGMT, ARC_NETMGMT_ACK,
                                payload, sizeof(payload));
  if (n > 0) notifyArcFrameToGs(frame, (size_t)n);
}

static void sendStatusReportToGs(uint8_t to) {
  arc_radio_status_report_t rep;
  rep.frequency_hz = (uint32_t)(g_curFreqMHz * 1.0e6f);
  rep.tx_power_dbm = kDefaultTxPowerDbm;  // no power readback in the driver yet
  rep.rssi_dbm = (int8_t)g_lastRssi;
  rep.snr_db = (int8_t)g_lastSnr;
  rep.error_flags = 0;
  rep.packets_rx = (uint16_t)g_downCount;
  rep.packets_tx = (uint16_t)g_loraTxCount;

  uint8_t payload[ARC_RADIO_STATUS_REPORT_PAYLOAD_SIZE];
  if (arc_radio_status_report_encode(&rep, payload, sizeof(payload)) < 0) return;

  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), ARC_ADDR_RADIO_G, to,
                                0, g_session, g_seq++,
                                ARC_FAMILY_RADIO, ARC_RADIO_STATUS_REPORT,
                                payload, sizeof(payload));
  if (n > 0) notifyArcFrameToGs(frame, (size_t)n);
}

// Emit our own NETMGMT heartbeat to the GS app over BLE so the ground radio
// (RADIO_G) shows up as a live node, independent of rocket downlink traffic.
static void sendSelfHeartbeatToGs() {
  uint8_t frame[ARC_MAX_FRAME_SIZE];
  const int n = arc_frame_build(frame, sizeof(frame), ARC_ADDR_RADIO_G, ARC_ADDR_BROADCAST,
                                0, g_session, g_seq++,
                                ARC_FAMILY_NETMGMT, ARC_NETMGMT_HEARTBEAT,
                                nullptr, 0);
  if (n > 0) notifyArcFrameToGs(frame, (size_t)n);
}

static void waitForUplinkSlot() {
  const uint32_t elapsed = millis() - g_lastRxMs;
  if (elapsed < kUplinkSlotOffsetMs) delay(kUplinkSlotOffsetMs - elapsed);
}

// One COBS frame arrived from the GS app (uplink).
static void handleUplinkFrame(const uint8_t *data, size_t n) {
  bool isHeartbeat = false;
  arc_frame_t f;
  if (arc_frame_parse(data, n, &f) == ARC_OK) {
    isHeartbeat =
        (f.family == ARC_FAMILY_NETMGMT && f.type == ARC_NETMGMT_HEARTBEAT);
    if (f.dst == ARC_ADDR_RADIO_G) {
      if (f.flags & ARC_FLAG_RELIABLE) {
        ackLocalReliableToGs(f);
      }
      if (f.family == ARC_FAMILY_RADIO && f.type == ARC_RADIO_GET_STATUS) {
        sendStatusReportToGs(f.src);
      }
      return;
    }

    // Arm the freq mirror when the GS commands the flight radio to retune.
    if (f.dst == ARC_ADDR_RADIO_CMD && f.family == ARC_FAMILY_RADIO &&
        f.type == ARC_RADIO_SET_FREQUENCY) {
      arc_radio_set_frequency_t msg;
      if (arc_radio_set_frequency_decode(f.payload, f.payload_len, &msg) == ARC_OK) {
        g_freqPendingTargetMHz = (float)msg.frequency_hz / 1.0e6f;
        g_freqPendingSeq = f.seq;
        g_freqPendingValid = true;
        USBSerial.printf("[ARC] uplink SET_FREQUENCY %.3f MHz seq=%u (mirror armed)\n",
                         g_freqPendingTargetMHz, f.seq);
      }
    }
    if (f.dst == ARC_ADDR_RADIO_CMD && f.family == ARC_FAMILY_RADIO &&
        f.type == ARC_RADIO_SET_PHY_PROFILE) {
      arc_radio_set_phy_profile_t msg;
      if (arc_radio_set_phy_profile_decode(f.payload, f.payload_len, &msg) == ARC_OK &&
          (msg.profile_id == ARC_RADIO_PHY_PROFILE_SAFE_BW125 ||
           msg.profile_id == ARC_RADIO_PHY_PROFILE_FAST_BW500)) {
        g_phyPendingTarget = msg.profile_id;
        g_phyPendingSeq = f.seq;
        g_phyPendingValid = true;
        USBSerial.printf("[ARC] uplink SET_PHY_PROFILE %u seq=%u (mirror armed)\n",
                         g_phyPendingTarget, f.seq);
      }
    }
  }
  // Heartbeats are deferrable liveness: hold only the latest, never queue them
  // ahead of commands and never spend the immediate window on one.
  if (isHeartbeat) {
    hbStore(data, n);
    return;
  }

  if (millis() - g_lastRxMs <= kImmediateUplinkWindowMs) {
    waitForUplinkSlot();
    loraTransmit(data, n);
    g_irqFlag = false;
    radio.recieve();
    return;
  }
  if (!upEnqueue(data, n)) USBSerial.println("[ARC] uplink command queue full, dropped");
}

// Feed one BLE byte into the COBS reassembler (single consumer: loop()).
static uint8_t g_cobs[ARC_MAX_ENCODED_SIZE + 4];
static size_t g_cobsLen = 0;
static void feedUplinkByte(uint8_t b) {
  if (b == 0x00) {
    if (g_cobsLen == 0) return;
    if (g_cobsLen >= sizeof(g_cobs)) { g_cobsLen = 0; return; }
    g_cobs[g_cobsLen++] = 0x00;  // arc_cobs_decode wants the trailing delimiter
    uint8_t dec[ARC_MAX_FRAME_SIZE];
    const int n = arc_cobs_decode(g_cobs, g_cobsLen, dec, sizeof(dec));
    g_cobsLen = 0;
    if (n > 0) handleUplinkFrame(dec, (size_t)n);
  } else if (g_cobsLen < sizeof(g_cobs)) {
    g_cobs[g_cobsLen++] = b;
  } else {
    g_cobsLen = 0;
  }
}

static void handleDownlink() {
  const size_t plen = radio.getPacketLength();
  uint8_t raw[ARC_MAX_FRAME_SIZE];
  if (plen == 0 || plen > sizeof(raw)) {
    uint8_t scratch[ARC_MAX_FRAME_SIZE];
    radio.readData(scratch, sizeof(scratch));
    radio.recieve();
    return;
  }
  const int st = radio.readData(raw, plen);
  if (st != RADIOLIB_ERR_NONE) {
    radio.recieve();
    return;
  }
  g_lastRxMs = millis();
  g_downCount++;
  const float rssi = radio.getRSSI();
  const float snr = radio.getSNR();
  g_lastRssi = rssi;
  g_lastSnr = snr;

  // Snoop the flight radio's ACK to fire our mirrored hop.
  arc_frame_t f;
  if (arc_frame_parse(raw, plen, &f) == ARC_OK) {
    if (f.family == ARC_FAMILY_NETMGMT && f.type == ARC_NETMGMT_ACK) {
      arc_netmgmt_ack_t ack;
      if (arc_netmgmt_ack_decode(f.payload, f.payload_len, &ack) == ARC_OK) {
        if (g_freqPendingValid && ack.seq == g_freqPendingSeq) {
          g_hopTargetMHz = g_freqPendingTargetMHz;
          g_hopAtMs = millis() + kFreqSwitchDelayMs;
          g_hopPending = true;
          g_freqPendingValid = false;
          USBSerial.printf("[ARC] ACK seq=%u -> ground hop scheduled\n", ack.seq);
        }
        if (g_phyPendingValid && ack.seq == g_phyPendingSeq) {
          g_phyHopTarget = g_phyPendingTarget;
          g_phyHopAtMs = millis() + kFreqSwitchDelayMs;
          g_phyHopPending = true;
          g_phyPendingValid = false;
          USBSerial.printf("[ARC] ACK seq=%u -> ground PHY profile switch scheduled\n", ack.seq);
        }
      }
    }
  }

  // Forward the downlink to the GS app (COBS-framed).
  notifyArcFrameToGs(raw, plen);
  USBSerial.printf("[LoRa] down %u B rssi=%.1f snr=%.1f\n", (unsigned)plen, rssi, snr);

  // Command window is open now -> send one queued command (preferred), or fall
  // back to the latest GS heartbeat so the rocket still learns the ground route.
  UpFrame up;
  if (upDequeue(up)) {
    waitForUplinkSlot();
    loraTransmit(up.data, up.len);
  } else if (g_hbPendingValid) {
    g_hbPendingValid = false;
    waitForUplinkSlot();
    loraTransmit(g_hbPending.data, g_hbPending.len);
  }
  g_irqFlag = false;
  radio.recieve();
}

static int upQueueDepth() {
  if (g_upHead >= g_upTail) return g_upHead - g_upTail;
  return kUpQDepth - g_upTail + g_upHead;
}

static void printDebug(uint32_t now) {
  if (now - g_lastDebugMs < 1000) return;
  g_lastDebugMs = now;
  USBSerial.printf("[DBG] ms=%lu ble_client=%d notify=%d irq=%lu poll=%lu down=%lu notify_bytes=%lu drop_bytes=%lu upq=%d freq=%.3f phy=%u rx_state=%d rx_rc=%d rx_start=%lu\n",
                   (unsigned long)now,
                   (int)g_hasClient,
                   (int)g_notifyEnabled,
                   (unsigned long)g_irqCount,
                   (unsigned long)g_pollCount,
                   (unsigned long)g_downCount,
                   (unsigned long)g_notifyBytes,
                   (unsigned long)g_notifyDropBytes,
                   upQueueDepth(),
                   g_curFreqMHz,
                   g_curPhyProfile,
                   radio.getState(),
                   radio.getLastReceiveRc(),
                   (unsigned long)radio.getReceiveStartCount());
}

static void applyHop() {
  const int rc = radio.setFrequency(g_hopTargetMHz);
  if (rc == RADIOLIB_ERR_NONE) {
    g_curFreqMHz = g_hopTargetMHz;
    USBSerial.printf("[LoRa] ground hopped to %.3f MHz\n", g_curFreqMHz);
  } else {
    USBSerial.printf("[LoRa] setFrequency err=%d\n", rc);
  }
  g_hopPending = false;
  g_lastRxMs = millis();
  radio.recieve();
}

static void applyPhyHop() {
  const int rc = radio.applyPhyProfile(g_phyHopTarget);
  if (rc == RADIOLIB_ERR_NONE) {
    g_curPhyProfile = g_phyHopTarget;
    USBSerial.printf("[LoRa] ground PHY profile -> %u\n", g_curPhyProfile);
  } else {
    USBSerial.printf("[LoRa] applyPhyProfile err=%d\n", rc);
  }
  g_phyHopPending = false;
  g_lastRxMs = millis();
  radio.recieve();
}

static void maybeRevert() {
  if (g_curFreqMHz == kDefaultFreqMHz && g_curPhyProfile == kDefaultPhyProfile) return;
  if (millis() - g_lastRxMs < kRevertTimeoutMs) return;
  USBSerial.printf("[LoRa] silent, ground reverting freq %.3f -> %.3f MHz profile %u -> %u\n",
                   g_curFreqMHz, kDefaultFreqMHz, g_curPhyProfile, kDefaultPhyProfile);
  radio.setFrequency(kDefaultFreqMHz);
  radio.applyPhyProfile(kDefaultPhyProfile);
  g_curFreqMHz = kDefaultFreqMHz;
  g_curPhyProfile = kDefaultPhyProfile;
  g_lastRxMs = millis();
  radio.recieve();
}

// ======================= Setup / Loop ==========================
void setup() {
  USBSerial.begin(115200);
  delay(2000);
  USBSerial.println("\n=== ARC ground radio (Type2GT LoRa <-> BLE NUS) ===");

  // Random session per boot so the ground app resets its dedup window.
  g_session = (uint8_t)(esp_random() & 0xFF);
  if (g_session == 0) g_session = 1;

  // ---- BLE ----
  NimBLEDevice::init("ARC-GS");
  NimBLEDevice::setMTU(247);
  g_bleServer = NimBLEDevice::createServer();
  g_bleServer->setCallbacks(new ServerCallbacks());
  g_bleServer->advertiseOnDisconnect(true);

  NimBLEService *svc = g_bleServer->createService(NUS_SERVICE_UUID);
  g_txChar = svc->createCharacteristic(NUS_TX_CHAR_UUID,
                                       NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  g_txChar->setCallbacks(new TxCallbacks());
  auto *rxChar = svc->createCharacteristic(
      NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());
  svc->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData ad;
  ad.setFlags(0x06);
  ad.addServiceUUID(NUS_SERVICE_UUID);
  adv->setAdvertisementData(ad);
  NimBLEAdvertisementData sd;
  sd.setName("ARC-GS");
  adv->setScanResponseData(sd);
  ble_start_advertising("initial start");

  // ---- LoRa (Type2GT) ---- begin() applies the shared PHY + RF-switch config.
  spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);
  const int rc = radio.begin();
  USBSerial.printf("[LoRa] begin -> %d\n", rc);
  radio.onIrq(onLoraIrq);
  const int rxRc = radio.recieve();
  USBSerial.printf("[LoRa] startReceive -> %d\n", rxRc);

  g_curFreqMHz = kDefaultFreqMHz;
  g_curPhyProfile = kDefaultPhyProfile;
  g_lastRxMs = millis();
}

void loop() {
  const uint32_t now = millis();

  // BLE uplink bytes -> COBS frames -> uplink queue
  int b;
  while ((b = bleRingPop()) >= 0) feedUplinkByte((uint8_t)b);

  // LoRa: service the IRQ out of interrupt context, then drain any packet
  if (g_irqFlag) {
    g_irqFlag = false;
    g_irqCount++;
    radio.respondToIrq();
  }
  if (now - g_lastPollMs >= 20) {
    g_lastPollMs = now;
    g_pollCount++;
  }
  if (radio.hasData()) handleDownlink();

  // Announce ourselves to the GS app so RADIO_G appears as a live node.
  if (g_notifyEnabled && now - g_lastSelfHeartbeatMs >= kSelfHeartbeatMs) {
    g_lastSelfHeartbeatMs = now;
    sendSelfHeartbeatToGs();
  }

  // mirrored frequency hop + safety revert
  printDebug(now);
  if (g_hopPending && (int32_t)(now - g_hopAtMs) >= 0) applyHop();
  if (g_phyHopPending && (int32_t)(now - g_phyHopAtMs) >= 0) applyPhyHop();
  maybeRevert();
}
