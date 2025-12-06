#include <Arduino.h>
#include <NimBLEDevice.h>
#include <RadioLib.h>
#include <RadioMessage.h>

// =============== Debug helpers (USB CDC on ESP32-S3) ===============
static void dbgHex(const uint8_t *d, size_t n)
{
    // for (size_t i = 0; i < n; ++i)
    // {
    //     if ((i & 0x0F) == 0)
    //         USBSerial.printf("\n  %04u: ", (unsigned)i);
    //     USBSerial.printf("%02X ", d[i]);
    // }
    // USBSerial.println();
}

static void dbgBanner(const char *step, int code = 0)
{
    // if (code == 0)
    //     USBSerial.printf("[DBG] %s\n", step);
    // else
    //     USBSerial.printf("[DBG] %s -> code=%d\n", step, code);
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
volatile bool g_loraOpDone = false;
volatile uint32_t g_irqCount = 0;

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
static void onLoraIrq()
{
    g_loraOpDone = true;
    g_irqCount++;
}

// ======================= BLE (NimBLE NUS) ======================
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify only

NimBLECharacteristic *g_txChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;

static void ble_notify_chunked(const char *data, size_t len)
{
    if (!g_hasClient || !g_notifyEnabled || !g_txChar || len == 0)
    {
        // USBSerial.printf("[BLE] notify skip (client=%d notify=%d char=%p len=%u)\n",
        //                  (int)g_hasClient, (int)g_notifyEnabled, (void *)g_txChar, (unsigned)len);
        return;
    }
    const uint16_t mtu = NimBLEDevice::getMTU();          // 23..247
    const size_t maxPayload = (mtu > 3) ? (mtu - 3) : 20; // ATT notif payload
    // USBSerial.printf("[BLE] notify len=%u mtu=%u chunk=%u\n",
    //                  (unsigned)len, (unsigned)mtu, (unsigned)maxPayload);

    for (size_t off = 0; off < len; off += maxPayload)
    {
        const size_t n = ((len - off) > maxPayload) ? maxPayload : (len - off);
        g_txChar->setValue((uint8_t *)(data + off), n);
        g_txChar->notify();
    }
}
class RxCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
    {
        const std::string &v = c->getValue();
        // USBSerial.printf("[BLE] RX wrote %u bytes\n", (unsigned)v.size());
        // no further action
    }
};

class TxCallbacks : public NimBLECharacteristicCallbacks
{
    void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override
    {
        g_notifyEnabled = (subValue & 0x0001);
        // USBSerial.printf("[BLE] TX subscribe 0x%04X -> notify=%d\n", subValue, (int)g_notifyEnabled);
    }
};

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override
    {
        g_hasClient = true;
        // USBSerial.println("[BLE] client connected");
    }
    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
    {
        // USBSerial.println("[BLE] client disconnected -> re-adv");
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
    // USBSerial.println("\n=== LoRa RX -> BLE NUS Bridge (verbose) ===");

    // ---- BLE ----
    dbgBanner("BLE init");
    NimBLEDevice::init("ESP32-NUS-3");
    NimBLEDevice::setPower(7);
    NimBLEDevice::setMTU(247);

    std::string addr = NimBLEDevice::getAddress().toString();
    // USBSerial.printf("[BLE] addr=%s\n", addr.c_str());

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
    // USBSerial.println("[BLE] advertising started (look for 'ESP32-NUS-3')");

    // ---- LoRa (LR11x0) ----
    // USBSerial.println("[LORA] SPI begin");
    // sck, miso, mosi, ss — keep your working pins
    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);

    // USBSerial.print("[LORA] radio.begin ... ");
    int st = radio.begin();
    // USBSerial.printf("ret=%d\n", st);
    if (st != RADIOLIB_ERR_NONE)
    {
        // USBSerial.println("[LORA] FATAL: radio.begin failed");
    }
    // after radio.begin(), before startReceive/startTransmit:

    int rc;
    rc = radio.setFrequency(915.0);
    // USBSerial.printf("[LORA] setFrequency -> %d\n", rc);
    rc = radio.setSpreadingFactor(7);
    // USBSerial.printf("[LORA] setSF -> %d\n", rc); // SF7
    rc = radio.setBandwidth(125.0);
    // USBSerial.printf("[LORA] setBW -> %d\n", rc); // 125 kHz
    rc = radio.setCodingRate(5);
    // USBSerial.printf("[LORA] setCR -> %d\n", rc); // 4/5
    rc = radio.setSyncWord(0x34);
    // USBSerial.printf("[LORA] setSync -> %d\n", rc); // classic LoRa
    rc = radio.setPreambleLength(8);
    // USBSerial.printf("[LORA] setPreamble -> %d\n", rc);
    rc = radio.setCRC(true);
    // USBSerial.printf("[LORA] setCRC -> %d\n", rc);
    // optional but nice:
    rc = radio.setOutputPower(14);
    // USBSerial.printf("[LORA] setPower -> %d\n", rc); // TX side only

    // USBSerial.println("[LORA] set RF switch table");
    radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

    // USBSerial.println("[LORA] set regulator DCDC");
    radio.setRegulatorDCDC();

    // USBSerial.println("[LORA] set IRQ action");
    radio.setIrqAction(onLoraIrq);
    radio.explicitHeader();
    radio.invertIQ(false);
    // USBSerial.print("[LORA] startReceive ... ");
    st = radio.startReceive();
    // USBSerial.printf("ret=%d\n", st);
    if (st != RADIOLIB_ERR_NONE)
    {
        // USBSerial.println("[LORA] WARNING: startReceive failed");
    }
}
bool foundNewline = false;
int bytesAvail = 0;
char serialBuf[Message::maxSize] = {0};
int serialBufLength = 0;
Message commandMsg;
APRSConfig commandConfig = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint16_t commandSize = 0;
enum InputState
{
    HANDSHAKE,
    COMMAND,
    // add additional states as necessary
    NONE
};

InputState currState = NONE;
GSData avionicsData(APRSTelem::type, 1, 3);
bool handshakeSuccess = false;
bool hasDataHeader = false;
bool hasAvionicsTelem = false;
Message m;
double arr[3] = {0.0, 0.0, 0.0};
void loop()
{

    if (((bytesAvail = USBSerial.available()) > 0))
    {
        // check if a command is being sent
        if (currState == NONE)
        {
            //   log("buf");
            for (int i = 0; i < bytesAvail; i++)
            {
                char c = USBSerial.read();
                if (c == '\n')
                {
                    serialBuf[serialBufLength] = 0;
                    foundNewline = true;
                    break;
                }
                if (c != '\0')
                {
                    serialBuf[serialBufLength] = c;
                    serialBufLength++;
                    if (serialBufLength >= (int)sizeof(serialBuf))
                        break;
                }
            }
            bytesAvail = USBSerial.available();

            if (foundNewline)
            {
                // log("newline command ", serialBuf);
                if (strcmp(serialBuf, "handshake") == 0)
                {
                    //   log("Starting handshake");
                    // begin the handshake
                    handshakeSuccess = false;
                    currState = HANDSHAKE;
                }
                else if (strcmp(serialBuf, "handshake succeeded") == 0)
                {
                    //   log("Successful handshake");
                    // the handshake was successful
                    handshakeSuccess = true;
                    // we will start sending data, so set up metrics
                    //   telemMetrics.setInitialTime(millis());
                }
                else if (strcmp(serialBuf, "handshake failed") == 0)
                {
                    //   log("Failed handshake");
                    // the handshake was not successful
                    handshakeSuccess = false;
                }
                else if (strcmp(serialBuf, "command") == 0)
                {
                    //   log("Ready for radio command");
                    // the next text will be a radio command
                    commandMsg.clear();
                    currState = COMMAND;
                }
                // add additional commands as required

                // reset serial buffer
                memset(serialBuf, 0, sizeof(serialBuf));
                serialBufLength = 0;
                foundNewline = false;
            }
        }
        // complete the handshake
    if (currState == HANDSHAKE)
    {
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = USBSerial.read();
        // log("read:");
        // skip odd characters that accidently get added
        if (c != 0xff && c != 0)
        {
          USBSerial.write(c);
          if (c == '\n')
          {
            // log("newline handshake");
            foundNewline = true;
            break;
          }
        }
      }
      bytesAvail = USBSerial.available();

      if (foundNewline)
      {
        foundNewline = false;
        currState = NONE;
      }
    }

    // receive the radio command
    if (currState == COMMAND)
    {
      // read into serial buffer
      for (int i = 0; i < bytesAvail; i++)
      {
        commandMsg.append(USBSerial.read());
        if (commandMsg.size >= Message::maxSize)
          break;
      }
      bytesAvail = USBSerial.available();
        radio.transmit("CMD/boop\n", sizeof("CMD/boop\n"));2 
        currState = NONE;
    }
    }
    // 1 Hz heartbeat with states
    static uint32_t lastHb = 0;
    uint32_t now = millis();
    if (now - lastHb >= 1000)
    {
        lastHb = now;
        // USBSerial.printf("[HB] heap=%u client=%d notify=%d irqCount=%u\n",
        //                  (unsigned)ESP.getFreeHeap(), (int)g_hasClient,
        //                  (int)g_notifyEnabled, (unsigned)g_irqCount);
    }

    // LoRa packet arrived
    if (g_loraOpDone)
    {
        g_loraOpDone = false;

        String payload;
        int st = radio.readData(payload);
        // USBSerial.printf("[LORA] readData ret=%d len=%u\n", st, (unsigned)payload.length());

        if (st == RADIOLIB_ERR_NONE)
        {
            // Debug RF metrics
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();
            // USBSerial.printf("[LORA] RSSI=%.1f dBm SNR=%.1f dB\n", rssi, snr);

            // USB hex (in case of control chars)
            if (payload.length())
            {
                // USBSerial.print("[LORA] payload (hex):");
                dbgHex((const uint8_t *)payload.c_str(), payload.length());
            }

            // BLE notify (newline-terminated for line-oriented clients)
            if (payload.length() == 0 || payload[payload.length() - 1] != '\n')
            {
                payload += '\n';
            }
            ble_notify_chunked(payload.c_str(), payload.length());

            double time, alt, vel, acc, x, y;
            sscanf(payload.c_str(), "TELEM2/%lf,%lf,%lf,%lf,%lf,%lf", &time, &alt, &vel, &acc, &y, &x);
            // USBSerial.printf("[LORA] parsed: time=%.2f alt=%.2f vel=%.2f acc=%.2f x=%.2f y=%.2f\n",
            //                  time, alt, vel, acc, y, x);
            APRSConfig con = {"N0CALL", "APRS", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '/', 0x5C};
            APRSTelem telem(con, y, x, alt, vel, 0.0, arr, 0x00000001);

            m.encode(&telem);
            // USBSerial.printf("[LORA] telem size=%u encoded size=%u\n",
            //                  (unsigned)m.size, (unsigned)m.size);
            // update metrics
            // telemMetrics.update(m.size, millis(), radioTelem.RSSI());
            // set the flag to transmit data
            hasAvionicsTelem = true;
        }
        else
        {
            // USBSerial.println("[LORA] readData failed");
        }

        // Return to RX (log result)
        st = radio.startReceive();
        // USBSerial.printf("[LORA] restart RX ret=%d\n", st);
        if (handshakeSuccess ||
         true)
        {
            // output goes here
            if (hasAvionicsTelem)
            {
                // fill GSData with message
                avionicsData.fill(m.buf, m.size);
                // encode for multplexing
                m.encode(&avionicsData);
                // USBSerial.printf("[AVIONICS] telem size=%u encoded size=%u\n",
                                //  (unsigned)avionicsData.size, (unsigned)m.size);
                // write
                // log("Avionics data: ", (const char *)m.buf);
                USBSerial.write(m.buf, m.size);
                // reset flag
                hasAvionicsTelem = false;
            }
        }
    }
}
