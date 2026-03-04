#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>
#include <string.h>

// ======================= LoRa (LR11x0) =========================
SPIClass spi(HSPI);
LR1121 radio = new Module(/*CS*/ 14, /*DIO1*/ 36, /*RST*/ 38, /*BUSY*/ 37, spi);

static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

enum LoraState : uint8_t
{
    LORA_IDLE = 0,
    LORA_TX = 1,
    LORA_RX = 2,
};

static const uint8_t EVENT_TX_DONE = 0x01;
static const uint8_t EVENT_RX_DONE = 0x02;
static const int ERR_RADIO_BUSY = -8001;
static const size_t MAX_RADIO_LINE = 220;
static const uint32_t ACK_TIMEOUT_MS = 450;
static const uint8_t MAX_ATTEMPTS = 4;

volatile LoraState g_loraState = LORA_IDLE;
volatile uint8_t g_loraEvents = 0;
volatile uint32_t g_irqCount = 0;
volatile uint32_t g_txStartUs = 0;
volatile uint32_t g_txIrqUs = 0;

// ======================= BLE (NUS) ======================
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

NimBLECharacteristic *g_txChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;

portMUX_TYPE g_bleMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool g_bleLineReady = false;
char g_bleLine[MAX_RADIO_LINE] = {0};

struct PendingMsg
{
    bool active;
    bool waitingAck;
    uint16_t seq;
    uint8_t attempts;
    uint32_t retryAtMs;
    char payload[MAX_RADIO_LINE];
};

PendingMsg pending = {false, false, 0, 0, 0, {0}};
uint16_t txSeq = 0;
uint16_t ackSeqQueued = 0;
uint16_t lastRxSeq = 0;
bool ackQueued = false;
bool hasLastRxSeq = false;

static bool startsWith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static void trimLine(char *line)
{
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
    {
        line[len - 1] = '\0';
        len--;
    }
}

static bool popRadioEvent(uint8_t &event)
{
    noInterrupts();
    const uint8_t events = g_loraEvents;

    if ((events & EVENT_TX_DONE) != 0)
    {
        g_loraEvents = (uint8_t)(g_loraEvents & ~EVENT_TX_DONE);
        interrupts();
        event = EVENT_TX_DONE;
        return true;
    }

    if ((events & EVENT_RX_DONE) != 0)
    {
        g_loraEvents = (uint8_t)(g_loraEvents & ~EVENT_RX_DONE);
        interrupts();
        event = EVENT_RX_DONE;
        return true;
    }

    interrupts();
    return false;
}

static int startRx()
{
    g_loraState = LORA_RX;
    int rc = radio.startReceive();
    if (rc != RADIOLIB_ERR_NONE)
    {
        g_loraState = LORA_IDLE;
    }
    return rc;
}

static int startTx(const char *frame)
{
    if (g_loraState == LORA_TX)
    {
        return ERR_RADIO_BUSY;
    }

    g_loraState = LORA_TX;
    g_txStartUs = micros();
    int rc = radio.startTransmit(frame);
    if (rc != RADIOLIB_ERR_NONE)
    {
        g_loraState = LORA_IDLE;
    }
    return rc;
}

static bool radioBusy()
{
    return g_loraState == LORA_TX;
}

static uint32_t lastTxDurationUs()
{
    noInterrupts();
    const uint32_t start = g_txStartUs;
    const uint32_t done = g_txIrqUs;
    interrupts();
    return (uint32_t)(done - start);
}

static void bleNotifyChunked(const char *data, size_t len)
{
    if (!g_hasClient || !g_notifyEnabled || !g_txChar || len == 0)
    {
        return;
    }

    const uint16_t mtu = NimBLEDevice::getMTU();
    const size_t maxPayload = (mtu > 3) ? (mtu - 3) : 20;
    for (size_t off = 0; off < len; off += maxPayload)
    {
        const size_t n = ((len - off) > maxPayload) ? maxPayload : (len - off);
        g_txChar->setValue((uint8_t *)(data + off), n);
        g_txChar->notify();
        if (off + n < len)
        {
            delayMicroseconds(500);
        }
    }
}

static void bleNotifyLine(const char *line)
{
    size_t len = strlen(line);
    if (len == 0)
    {
        return;
    }

    if (line[len - 1] == '\n')
    {
        bleNotifyChunked(line, len);
        return;
    }

    char buf[MAX_RADIO_LINE + 2];
    size_t n = (len > MAX_RADIO_LINE) ? MAX_RADIO_LINE : len;
    memcpy(buf, line, n);
    buf[n++] = '\n';
    buf[n] = '\0';
    bleNotifyChunked(buf, n);
}

static void queueMessage(const char *payload)
{
    if (pending.active)
    {
        USBSerial.println("[RAD] pending TX waiting for ACK, dropping new line");
        return;
    }

    pending.active = true;
    pending.waitingAck = false;
    pending.seq = ++txSeq;
    pending.attempts = 0;
    pending.retryAtMs = millis();
    strncpy(pending.payload, payload, sizeof(pending.payload) - 1);
    pending.payload[sizeof(pending.payload) - 1] = '\0';
}

static void maybeSendAck()
{
    if (!ackQueued || radioBusy())
    {
        return;
    }

    char frame[32];
    snprintf(frame, sizeof(frame), "RAD/ACK,%u", (unsigned)ackSeqQueued);
    int rc = startTx(frame);
    if (rc == RADIOLIB_ERR_NONE)
    {
        ackQueued = false;
    }
}

static void maybeSendMessage()
{
    if (!pending.active || pending.waitingAck || radioBusy())
    {
        return;
    }

    if ((int32_t)(millis() - pending.retryAtMs) < 0)
    {
        return;
    }

    if (pending.attempts >= MAX_ATTEMPTS)
    {
        USBSerial.printf("[RAD] ERROR seq=%u failed after %u attempts\n",
                         (unsigned)pending.seq, (unsigned)pending.attempts);
        pending.active = false;
        return;
    }

    char frame[320];
    snprintf(frame, sizeof(frame), "RAD/MSG,%u,%s", (unsigned)pending.seq, pending.payload);
    int rc = startTx(frame);
    if (rc == RADIOLIB_ERR_NONE)
    {
        pending.attempts++;
        pending.waitingAck = true;
        pending.retryAtMs = millis() + ACK_TIMEOUT_MS;
        USBSerial.printf("[RAD] TX seq=%u attempt=%u\n", (unsigned)pending.seq, (unsigned)pending.attempts);
    }
}

static void handleAckFrame(const char *frame)
{
    unsigned int ackSeq = 0;
    if (sscanf(frame, "RAD/ACK,%u", &ackSeq) != 1)
    {
        return;
    }

    USBSerial.printf("[RAD] RX ACK,%u\n", ackSeq);
    if (pending.active && pending.waitingAck && pending.seq == (uint16_t)ackSeq)
    {
        pending.active = false;
        pending.waitingAck = false;
        USBSerial.printf("[RAD] ACK matched seq=%u\n", ackSeq);
    }
}

static void handleMsgFrame(const char *frame)
{
    unsigned int rxSeq = 0;
    int payloadOffset = 0;
    if (sscanf(frame, "RAD/MSG,%u,%n", &rxSeq, &payloadOffset) != 1 || payloadOffset <= 0)
    {
        return;
    }

    const char *payload = frame + payloadOffset;
    const bool duplicate = hasLastRxSeq && lastRxSeq == (uint16_t)rxSeq;
    if (!duplicate)
    {
        lastRxSeq = (uint16_t)rxSeq;
        hasLastRxSeq = true;
        USBSerial.printf("[RAD] RX MSG,%u,%s\n", rxSeq, payload);
        bleNotifyLine(payload);
    }

    ackSeqQueued = (uint16_t)rxSeq;
    ackQueued = true;
}

static void handleRxFrame(const char *frame)
{
    if (startsWith(frame, "RAD/ACK,"))
    {
        handleAckFrame(frame);
    }
    else if (startsWith(frame, "RAD/MSG,"))
    {
        handleMsgFrame(frame);
    }
    else
    {
        USBSerial.printf("[RAD] RX RAW,%s\n", frame);
        bleNotifyLine(frame);
    }
}

static void processLocalLine(const char *line)
{
    if (startsWith(line, "RAD/PING"))
    {
        USBSerial.println("RAD/PONG");
        bleNotifyLine("RAD/PONG");
        return;
    }

    if (startsWith(line, "RAD/STATUS"))
    {
        char status[128];
        snprintf(status, sizeof(status),
                 "RAD/Status active=%d waitingAck=%d attempts=%u ackQueued=%d busy=%d",
                 pending.active ? 1 : 0,
                 pending.waitingAck ? 1 : 0,
                 (unsigned)pending.attempts,
                 ackQueued ? 1 : 0,
                 radioBusy() ? 1 : 0);
        USBSerial.println(status);
        bleNotifyLine(status);
        return;
    }

    queueMessage(line);
}

static void pollUSBSerial()
{
    while (USBSerial.available())
    {
        char line[MAX_RADIO_LINE];
        int n = USBSerial.readBytesUntil('\n', line, sizeof(line) - 1);
        if (n <= 0)
        {
            break;
        }
        line[n] = '\0';
        trimLine(line);
        if (strlen(line) == 0)
        {
            continue;
        }
        processLocalLine(line);
    }
}

static void pollBleLine()
{
    if (!g_bleLineReady)
    {
        return;
    }

    char line[MAX_RADIO_LINE];
    portENTER_CRITICAL(&g_bleMux);
    strncpy(line, g_bleLine, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    g_bleLineReady = false;
    g_bleLine[0] = '\0';
    portEXIT_CRITICAL(&g_bleMux);

    trimLine(line);
    if (strlen(line) == 0)
    {
        return;
    }
    processLocalLine(line);
}

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
static void onLoraIrq()
{
    if (g_loraState == LORA_TX)
    {
        g_txIrqUs = micros();
        g_loraEvents = (uint8_t)(g_loraEvents | EVENT_TX_DONE);
        g_loraState = LORA_IDLE;
    }
    else if (g_loraState == LORA_RX || g_loraState == LORA_IDLE)
    {
        g_loraEvents = (uint8_t)(g_loraEvents | EVENT_RX_DONE);
        g_loraState = LORA_IDLE;
    }
    g_irqCount++;
}

class RxCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
    {
        const std::string &v = c->getValue();
        if (v.empty())
        {
            return;
        }

        bool queued = false;
        portENTER_CRITICAL(&g_bleMux);
        if (!g_bleLineReady)
        {
            const size_t n = (v.size() < (MAX_RADIO_LINE - 1)) ? v.size() : (MAX_RADIO_LINE - 1);
            memcpy(g_bleLine, v.data(), n);
            g_bleLine[n] = '\0';
            g_bleLineReady = true;
            queued = true;
        }
        portEXIT_CRITICAL(&g_bleMux);

        if (!queued)
        {
            USBSerial.println("[BLE] input dropped (previous line pending)");
        }
    }
};

class TxCallbacks : public NimBLECharacteristicCallbacks
{
    void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override
    {
        g_notifyEnabled = (subValue & 0x0001) != 0;
        USBSerial.printf("[BLE] subscribe 0x%04X notify=%d\n", subValue, (int)g_notifyEnabled);
    }
};

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override
    {
        g_hasClient = true;
        USBSerial.println("[BLE] client connected");
    }

    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
    {
        g_hasClient = false;
        g_notifyEnabled = false;
        USBSerial.println("[BLE] client disconnected -> re-advertise");
        NimBLEDevice::startAdvertising();
    }
};

void setup()
{
    USBSerial.begin(115200);
    delay(1200);
    USBSerial.println("\n=== ESP32FC LoRa <-> BLE transceiver bridge ===");

    NimBLEDevice::init("ESP32-NUS-3");
    NimBLEDevice::setPower(7);
    NimBLEDevice::setMTU(247);

    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());
    NimBLEService *svc = server->createService(NUS_SERVICE_UUID);

    g_txChar = svc->createCharacteristic(
        NUS_TX_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    g_txChar->setCallbacks(new TxCallbacks());

    NimBLECharacteristic *rxChar = svc->createCharacteristic(
        NUS_RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rxChar->setCallbacks(new RxCallbacks());

    svc->start();
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData ad;
    ad.setFlags(0x06);
    ad.addServiceUUID(NUS_SERVICE_UUID);
    adv->setAdvertisementData(ad);
    NimBLEAdvertisementData sd;
    sd.setName("ESP32-NUS-3");
    adv->setScanResponseData(sd);
    adv->start();
    USBSerial.println("[BLE] advertising started");

    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);

    int rc = radio.begin();
    USBSerial.printf("[LORA] begin -> %d\n", rc);
    if (rc != RADIOLIB_ERR_NONE)
    {
        USBSerial.println("[LORA] FATAL: begin failed");
    }

    rc = radio.setFrequency(915.0);
    USBSerial.printf("[LORA] setFrequency -> %d\n", rc);
    rc = radio.setSpreadingFactor(7);
    USBSerial.printf("[LORA] setSF -> %d\n", rc);
    rc = radio.setBandwidth(250.0);
    USBSerial.printf("[LORA] setBW -> %d\n", rc);
    rc = radio.setCodingRate(5);
    USBSerial.printf("[LORA] setCR -> %d\n", rc);
    rc = radio.setSyncWord(0x12);
    USBSerial.printf("[LORA] setSync -> %d\n", rc);
    rc = radio.setPreambleLength(8);
    USBSerial.printf("[LORA] setPreamble -> %d\n", rc);
    rc = radio.setCRC(true);
    USBSerial.printf("[LORA] setCRC -> %d\n", rc);
    rc = radio.setOutputPower(14);
    USBSerial.printf("[LORA] setPower -> %d\n", rc);

    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    radio.setRegulatorDCDC();
    radio.explicitHeader();
    radio.invertIQ(false);
    radio.setIrqAction(onLoraIrq);

    rc = startRx();
    USBSerial.printf("[LORA] startReceive -> %d\n", rc);
    USBSerial.println("[RAD] transceiver ready");
}

void loop()
{
    static uint32_t lastHeartbeatMs = 0;
    const uint32_t now = millis();
    if ((uint32_t)(now - lastHeartbeatMs) >= 1000)
    {
        lastHeartbeatMs = now;
        USBSerial.printf("[HB] heap=%u client=%d notify=%d irq=%u\n",
                         (unsigned)ESP.getFreeHeap(),
                         (int)g_hasClient,
                         (int)g_notifyEnabled,
                         (unsigned)g_irqCount);
    }

    uint8_t event = 0;
    while (popRadioEvent(event))
    {
        if (event == EVENT_TX_DONE)
        {
            USBSerial.printf("[RAD] TX done in %.2f ms\n", lastTxDurationUs() / 1000.0f);
            int rc = startRx();
            if (rc != RADIOLIB_ERR_NONE)
            {
                USBSerial.printf("[LORA] startReceive after TX failed -> %d\n", rc);
            }
        }
        else if (event == EVENT_RX_DONE)
        {
            String payload;
            int rc = radio.readData(payload);
            if (rc == RADIOLIB_ERR_NONE)
            {
                float rssi = radio.getRSSI();
                float snr = radio.getSNR();
                USBSerial.printf("[LORA] RX len=%u RSSI=%.1f SNR=%.1f\n",
                                 (unsigned)payload.length(), rssi, snr);
                handleRxFrame(payload.c_str());
            }
            else
            {
                USBSerial.printf("[LORA] readData failed -> %d\n", rc);
            }

            rc = startRx();
            if (rc != RADIOLIB_ERR_NONE)
            {
                USBSerial.printf("[LORA] restart RX failed -> %d\n", rc);
            }
        }
    }

    if (pending.active && pending.waitingAck && (int32_t)(millis() - pending.retryAtMs) >= 0)
    {
        pending.waitingAck = false;
        pending.retryAtMs = millis();
        USBSerial.printf("[RAD] ACK timeout seq=%u, retrying\n", (unsigned)pending.seq);
    }

    pollBleLine();
    pollUSBSerial();
    maybeSendAck();
    maybeSendMessage();
}
