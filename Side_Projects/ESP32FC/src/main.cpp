#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>

// =============== Debug helpers (USB CDC on ESP32-S3) ===============
static void dbgHex(const uint8_t *d, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        if ((i & 0x0F) == 0)
            USBSerial.printf("\n  %04u: ", (unsigned)i);
        USBSerial.printf("%02X ", d[i]);
    }
    USBSerial.println();
}

static void dbgBanner(const char *step, int code = 0)
{
    if (code == 0)
        USBSerial.printf("[DBG] %s\n", step);
    else
        USBSerial.printf("[DBG] %s -> code=%d\n", step, code);
}

// ======================= LoRa (LR11x0) =========================
// SPI & LR11x0 pinout (match your wiring)
SPIClass spi(HSPI);
// Module(cs, dio1, rst, busy, spi)
LR1121 radio = new Module(/*CS*/ 14, /*DIO1*/ 36, /*RST*/ 38, /*BUSY*/ 37, spi);

// Optional RF switch control (use your known good table)
static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6, RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};
static const Module::RfSwitchMode_t rfswitch_table[] = {
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

// IRQ flag from RadioLib
volatile uint32_t g_loraOpDone = 0; // count of pending packets, not just a flag
volatile uint32_t g_irqCount = 0;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
static void onLoraIrq()
{
    g_loraOpDone++;
    g_irqCount++;
}

// ======================= BLE (NimBLE NUS) ======================
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify only

NimBLEServer *g_bleServer = nullptr;
NimBLECharacteristic *g_txChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;
static constexpr size_t kBleNotifyChunkBytes = 240;
static constexpr size_t kMaxRadioPacketBytes = 255;

static void ble_start_advertising(const char *reason)
{
    bool ok = g_bleServer ? g_bleServer->startAdvertising() : NimBLEDevice::startAdvertising();
    USBSerial.printf("[BLE] advertising %s -> %s\n", reason, ok ? "ok" : "failed");
}

static void ble_notify_forward(const uint8_t *data, size_t len)
{
    if (!g_hasClient || !g_notifyEnabled || !g_txChar || len == 0)
    {
        USBSerial.printf("[BLE] notify skip (client=%d notify=%d char=%p len=%u)\n",
                         (int)g_hasClient, (int)g_notifyEnabled, (void *)g_txChar, (unsigned)len);
        return;
    }
    g_txChar->setValue((uint8_t *)data, len);
    g_txChar->notify();
}

static void ble_notify_forward_chunked(const uint8_t *data, size_t len)
{
    while (len > 0)
    {
        const size_t chunkLen = len > kBleNotifyChunkBytes ? kBleNotifyChunkBytes : len;
        ble_notify_forward(data, chunkLen);
        data += chunkLen;
        len -= chunkLen;
    }
}

class RxCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
    {
        const std::string &v = c->getValue();
        USBSerial.printf("[BLE] RX wrote %u bytes\n", (unsigned)v.size());
        // no further action
    }
};

class TxCallbacks : public NimBLECharacteristicCallbacks
{
    void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override
    {
        g_notifyEnabled = (subValue & 0x0001);
        USBSerial.printf("[BLE] TX subscribe 0x%04X -> notify=%d\n", subValue, (int)g_notifyEnabled);
    }
};

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *server, NimBLEConnInfo &connInfo) override
    {
        g_hasClient = true;
        USBSerial.printf("[BLE] client connected handle=%u peers=%u mtu=%u\n",
                         (unsigned)connInfo.getConnHandle(),
                         (unsigned)server->getConnectedCount(),
                         (unsigned)server->getPeerMTU(connInfo.getConnHandle()));
    }

    void onDisconnect(NimBLEServer *server, NimBLEConnInfo &connInfo, int reason) override
    {
        g_hasClient = false;
        g_notifyEnabled = false;
        USBSerial.printf("[BLE] client disconnected handle=%u reason=%d peers=%u\n",
                         (unsigned)connInfo.getConnHandle(),
                         reason,
                         (unsigned)server->getConnectedCount());
        ble_start_advertising("restart after disconnect");
    }
};

// ======================= Setup / Loop ==========================
void setup()
{
    // USB CDC debug
    USBSerial.begin(115200);
    delay(3000);
    USBSerial.println("\n=== LoRa Raw Packet -> BLE NUS Bridge (verbose) ===");

    // ---- BLE ----
    dbgBanner("BLE init");
    NimBLEDevice::init("ESP32-NUS-3");
    NimBLEDevice::setPower(7);
    NimBLEDevice::setMTU(247);

    std::string addr = NimBLEDevice::getAddress().toString();
    USBSerial.printf("[BLE] addr=%s\n", addr.c_str());

    dbgBanner("create server");
    g_bleServer = NimBLEDevice::createServer();
    g_bleServer->setCallbacks(new ServerCallbacks());
    g_bleServer->advertiseOnDisconnect(true);
    USBSerial.println("[BLE] advertiseOnDisconnect enabled");

    dbgBanner("create service");
    NimBLEService *svc = g_bleServer->createService(NUS_SERVICE_UUID);

    dbgBanner("create TX characteristic");
    g_txChar = svc->createCharacteristic(
        NUS_TX_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    g_txChar->setCallbacks(new TxCallbacks());

    auto *rxChar = svc->createCharacteristic(
        NUS_RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR // Flutter usually wants WRITE_NR
    );
    rxChar->setCallbacks(new RxCallbacks());

    dbgBanner("start service");
    svc->start();

    dbgBanner("start advertising");
    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    NimBLEAdvertisementData ad;
    ad.setFlags(0x06);
    ad.addServiceUUID(NUS_SERVICE_UUID);
    adv->setAdvertisementData(ad);
    NimBLEAdvertisementData sd;
    sd.setName("ESP32-NUS-3");
    adv->setScanResponseData(sd);
    ble_start_advertising("initial start");

    // ---- LoRa (LR11x0) ----
    USBSerial.println("[LORA] SPI begin");
    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);

    USBSerial.print("[LORA] radio.begin ... ");
    int st = radio.begin();
    USBSerial.printf("ret=%d\n", st);
    if (st != RADIOLIB_ERR_NONE)
    {
        USBSerial.println("[LORA] FATAL: radio.begin failed");
    }

    int rc;
    rc = radio.setFrequency(915.0);
    USBSerial.printf("[LORA] setFrequency -> %d\n", rc);
    rc = radio.setSpreadingFactor(7);
    USBSerial.printf("[LORA] setSF -> %d\n", rc);
    rc = radio.setBandwidth(125.0);
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

    USBSerial.println("[LORA] set RF switch table");
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

    USBSerial.println("[LORA] set regulator DCDC");
    radio.setRegulatorDCDC();

    USBSerial.println("[LORA] set IRQ action");
    radio.setIrqAction(onLoraIrq);
    radio.explicitHeader();
    radio.invertIQ(false);
    USBSerial.print("[LORA] startReceive ... ");
    st = radio.startReceive();
    USBSerial.printf("ret=%d\n", st);
    if (st != RADIOLIB_ERR_NONE)
    {
        USBSerial.println("[LORA] WARNING: startReceive failed");
    }
}

void loop()
{
    // 1 Hz heartbeat
    static uint32_t lastHb = 0;
    uint32_t now = millis();
    if (now - lastHb >= 1000)
    {
        lastHb = now;
        USBSerial.printf("[HB] heap=%u client=%d notify=%d irqCount=%u\n",
                         (unsigned)ESP.getFreeHeap(), (int)g_hasClient,
                         (int)g_notifyEnabled, (unsigned)g_irqCount);
    }

    while (g_loraOpDone > 0)
    {
        g_loraOpDone--;

        uint8_t payload[kMaxRadioPacketBytes] = {};
        size_t packetLen = radio.getPacketLength();
        if (packetLen == 0 || packetLen > sizeof(payload))
        {
            USBSerial.printf("[LORA] unexpected packet length=%u\n", (unsigned)packetLen);
            int drainStatus = radio.readData(payload, sizeof(payload));
            USBSerial.printf("[LORA] drain read ret=%d\n", drainStatus);
            continue;
        }

        int st = radio.readData(payload, packetLen);
        int rx_restart_status = radio.startReceive();
        if (rx_restart_status != RADIOLIB_ERR_NONE)
        {
            USBSerial.printf("[LORA] restart RX ret=%d\n", rx_restart_status);
        }
        USBSerial.printf("[LORA] readData ret=%d len=%u\n", st, (unsigned)packetLen);

        if (st == RADIOLIB_ERR_NONE)
        {
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();
            USBSerial.printf("[LORA] RSSI=%.1f dBm SNR=%.1f dB\n", rssi, snr);
            USBSerial.printf("[LORA] header type=%u payloadLen=%u\n",
                             (unsigned)payload[0],
                             packetLen > 1 ? (unsigned)payload[1] : 0U);
            ble_notify_forward_chunked(payload, packetLen);
        }
        else
        {
            USBSerial.println("[LORA] readData failed");
        }
    }
}
