#include <NimBLEDevice.h>
#include <HardwareSerial.h>
#include <cstring>

// ===== UART on RX0/TX0 (ESP32-S3: GPIO44/43) =====
static const uint32_t UART_BAUD = 115200;
static const int UART_RX_GPIO = 44; // RX0 pad (module pin 36)
static const int UART_TX_GPIO = 43; // TX0 pad (module pin 37; unused but must be valid)
HardwareSerial SensorUart(0);       // UART0

// ===== NUS UUIDs =====
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write from phone
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify to phone

// ===== State =====
NimBLECharacteristic *pTxChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;

// ---------- Helpers ----------
// Forward one BLE notification without app-level chunk splitting.
static void notify_raw(const char *data, size_t len)
{
  if (!g_hasClient || !g_notifyEnabled || !pTxChar || len == 0)
    return;
  pTxChar->setValue((uint8_t *)data, len);
  pTxChar->notify();
}

// ---------- Callbacks ----------
class RxCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
  {
    // Forward phone->ESP write straight to STM32 UART exactly as received.
    const std::string &v = c->getValue();
    if (!v.empty())
    {
      SensorUart.write((const uint8_t *)v.data(), v.size());
    }
  }
};

class TxCallbacks : public NimBLECharacteristicCallbacks
{
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override
  {
    g_notifyEnabled = (subValue & 0x0001);
  }
};

class ServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *, NimBLEConnInfo &) override
  {
    g_hasClient = true;
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
  {
    g_hasClient = false;
    g_notifyEnabled = false;
    NimBLEDevice::startAdvertising();
  }
};

// ---------- UART line pump (normalize CRLF, forward lines verbatim) ----------
static const size_t RBUF_MAX = 1024;
static char rbuf[RBUF_MAX];
static size_t rlen = 0;

static void pump_uart()
{
  while (SensorUart.available())
  {
    const char c = (char)SensorUart.read();
    if (c == '\r')
      continue; // normalize CRLF to '\n'
    if (rlen < RBUF_MAX - 1)
      rbuf[rlen++] = c;

    // Dispatch on newline or buffer full
    if (c == '\n' || rlen == RBUF_MAX - 1)
    {
      rbuf[rlen] = '\0';

      // Forward UART line as-is, no filtering or prefixing.
      notify_raw(rbuf, rlen);

      rlen = 0;
    }
  }
}

// ---------- Arduino entry points ----------
void setup()
{
  // UART from STM32
  SensorUart.begin(UART_BAUD, SERIAL_8N1, UART_RX_GPIO, UART_TX_GPIO);

  // BLE init
  NimBLEDevice::init("ESP32-NUS-3");
  NimBLEDevice::setPower(7); // dBm
  NimBLEDevice::setMTU(247); // allow longer packets

  auto *server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  auto *svc = server->createService(NUS_SERVICE_UUID);

  pTxChar = svc->createCharacteristic(NUS_TX_CHAR_UUID,
                                      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pTxChar->setCallbacks(new TxCallbacks());

  auto *rx = svc->createCharacteristic(NUS_RX_CHAR_UUID,
                                       NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(new RxCallbacks());

  svc->start();

  auto *adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData ad;
  ad.setFlags(0x06);
  ad.addServiceUUID(NUS_SERVICE_UUID);
  adv->setAdvertisementData(ad);

  NimBLEAdvertisementData sd;
  sd.setName("ESP32-NUS-3");
  adv->setScanResponseData(sd);
  adv->start();
}

void loop()
{
  pump_uart();
}
