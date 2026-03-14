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
volatile uint32_t g_loraOpDone = 0;  // count of pending packets, not just a flag
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

NimBLECharacteristic *g_txChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;

static void ble_notify_forward(const char *data, size_t len)
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

static bool extract_chunk_data(const String &payload, String &chunkData)
{
    if (!payload.startsWith("CH/"))
    {
        chunkData = payload;
        return false;
    }

    const int p1 = payload.indexOf('/', 3);
    const int p2 = (p1 >= 0) ? payload.indexOf('/', p1 + 1) : -1;
    const int p3 = (p2 >= 0) ? payload.indexOf('/', p2 + 1) : -1;
    if (p3 < 0)
    {
        chunkData = payload;
        return false;
    }

    chunkData = payload.substring(p3 + 1);
    return true;
}

static bool is_complete_ctlm_line(const String &line)
{
    if (!line.startsWith("CTLM/"))
    {
        return false;
    }

    int commas = 0;
    for (size_t i = 0; i < line.length(); ++i)
    {
        if (line[i] == ',')
        {
            commas++;
        }
    }
    return commas == 14; // CTLM/<15 fields total>
}

static void forward_ctlm_records(const String &rxPayload)
{
    static String buf;
    static String lastForwarded;

    String chunk;
    extract_chunk_data(rxPayload, chunk);
    buf += chunk;

    if (buf.length() > 2048)
    {
        const int keepFrom = buf.lastIndexOf("CTLM/");
        if (keepFrom >= 0)
        {
            buf = buf.substring(keepFrom);
        }
        else
        {
            buf = "";
        }
    }

    while (true)
    {
        const int start = buf.indexOf("CTLM/");
        if (start < 0)
        {
            if (buf.length() > 256)
            {
                buf = "";
            }
            return;
        }

        if (start > 0)
        {
            buf.remove(0, start);
        }

        int next = buf.indexOf("CTLM/", 5);
        if (next < 0)
        {
            // End-of-line fallback
            const int nl = buf.indexOf('\n');
            if (nl >= 0)
            {
                String line = buf.substring(0, nl);
                line.trim();
                if (is_complete_ctlm_line(line) && line != lastForwarded)
                {
                    line += '\n';
                    USBSerial.printf("[LORA] forward CTLM len=%u\n", (unsigned)line.length());
                    ble_notify_forward(line.c_str(), line.length());
                    line.trim();
                    lastForwarded = line;
                }
                buf.remove(0, nl + 1);
                continue;
            }

            // If this is already a complete record without a trailing separator, forward it.
            if (is_complete_ctlm_line(buf) && buf != lastForwarded)
            {
                String out = buf + '\n';
                USBSerial.printf("[LORA] forward CTLM len=%u\n", (unsigned)out.length());
                ble_notify_forward(out.c_str(), out.length());
                lastForwarded = buf;
                buf = "";
            }
            return;
        }

        String candidate = buf.substring(0, next);
        candidate.trim();
        if (is_complete_ctlm_line(candidate) && candidate != lastForwarded)
        {
            String out = candidate + '\n';
            USBSerial.printf("[LORA] forward CTLM len=%u\n", (unsigned)out.length());
            ble_notify_forward(out.c_str(), out.length());
            lastForwarded = candidate;
        }
        buf.remove(0, next);
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
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override
    {
        g_hasClient = true;
        USBSerial.println("[BLE] client connected");
    }
    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
    {
        USBSerial.println("[BLE] client disconnected -> re-adv");
        g_hasClient = false;
        g_notifyEnabled = false;
        NimBLEDevice::startAdvertising();
    }
};

// ======================= Setup / Loop ==========================
void setup()
{
    // USB CDC debug
    USBSerial.begin(115200);
    delay(3000);
    USBSerial.println("\n=== LoRa RX -> BLE NUS Bridge (verbose) ===");

    // ---- BLE ----
    dbgBanner("BLE init");
    NimBLEDevice::init("ESP32-NUS-3");
    NimBLEDevice::setPower(7);
    NimBLEDevice::setMTU(247);

    std::string addr = NimBLEDevice::getAddress().toString();
    USBSerial.printf("[BLE] addr=%s\n", addr.c_str());

    dbgBanner("create server");
    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    dbgBanner("create service");
    NimBLEService *svc = server->createService(NUS_SERVICE_UUID);

    dbgBanner("create TX characteristic");
    g_txChar = svc->createCharacteristic(
        NUS_TX_CHAR_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    g_txChar->setCallbacks(new TxCallbacks());

    auto *rxChar = svc->createCharacteristic(
        NUS_RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR // Flutter usually wants WRITE_NR
    );
    // optional logging:
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
    adv->start();
    USBSerial.println("[BLE] advertising started (look for 'ESP32-NUS-3')");

    // ---- LoRa (LR11x0) ----
    USBSerial.println("[LORA] SPI begin");
    // sck, miso, mosi, ss — keep your working pins
    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);

    USBSerial.print("[LORA] radio.begin ... ");
    int st = radio.begin();
    USBSerial.printf("ret=%d\n", st);
    if (st != RADIOLIB_ERR_NONE)
    {
        USBSerial.println("[LORA] FATAL: radio.begin failed");
    }
    // after radio.begin(), before startReceive/startTransmit:

    int rc;
    rc = radio.setFrequency(915.0);
    USBSerial.printf("[LORA] setFrequency -> %d\n", rc);
    rc = radio.setSpreadingFactor(7);
    USBSerial.printf("[LORA] setSF -> %d\n", rc); // SF7
    rc = radio.setBandwidth(125.0);
    USBSerial.printf("[LORA] setBW -> %d\n", rc); // 125 kHz
    rc = radio.setCodingRate(5);
    USBSerial.printf("[LORA] setCR -> %d\n", rc); // 4/5
    rc = radio.setSyncWord(0x12);
    USBSerial.printf("[LORA] setSync -> %d\n", rc); // classic LoRa
    rc = radio.setPreambleLength(8);
    USBSerial.printf("[LORA] setPreamble -> %d\n", rc);
    rc = radio.setCRC(true);
    USBSerial.printf("[LORA] setCRC -> %d\n", rc);
    // optional but nice:
    rc = radio.setOutputPower(14);
    USBSerial.printf("[LORA] setPower -> %d\n", rc); // TX side only

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

        String payload;
        int st = radio.readData(payload);
        // Re-arm RX immediately.
        int rx_restart_status = radio.startReceive();
        if (rx_restart_status != RADIOLIB_ERR_NONE)
        {
            USBSerial.printf("[LORA] restart RX ret=%d\n", rx_restart_status);
        }
        USBSerial.printf("[LORA] readData ret=%d len=%u\n", st, (unsigned)payload.length());

        if (st == RADIOLIB_ERR_NONE)
        {
            // Debug RF metrics
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();
            USBSerial.printf("[LORA] RSSI=%.1f dBm SNR=%.1f dB\n", rssi, snr);
            USBSerial.printf("[LORA] payload: %s\n", payload.c_str());
            forward_ctlm_records(payload);
        }
        else
        {
            USBSerial.println("[LORA] readData failed");
        }
    }
}

