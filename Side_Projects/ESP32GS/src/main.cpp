#include <NimBLEDevice.h>
#include <HardwareSerial.h>
#include <cstring>
#include <cstdarg>

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
static void notify_chunked(const char *data, size_t len)
{
  if (!g_hasClient || !g_notifyEnabled || !pTxChar || len == 0)
    return;
  const uint16_t mtu = NimBLEDevice::getMTU();          // 23..247 (default 23 unless negotiated)
  const size_t maxPayload = (mtu > 3) ? (mtu - 3) : 20; // ATT notif payload size
  for (size_t off = 0; off < len; off += maxPayload)
  {
    const size_t n = ((len - off) > maxPayload) ? maxPayload : (len - off);
    pTxChar->setValue((uint8_t *)(data + off), n);
    pTxChar->notify();
  }
}

static void bleLogf(const char *fmt, ...)
{
  if (!g_hasClient || !g_notifyEnabled)
    return;
  static char buf[256];
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n > 0)
    notify_chunked(buf, (size_t)n);
}

static inline bool is_gps_line(const char *s)
{
  // Treat as GPS if it starts with "GPS," or "GPS "
  return (strncmp(s, "GPS,", 4) == 0) || (strncmp(s, "GPS ", 4) == 0);
}

// ---------- Callbacks ----------
class RxCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
  {
    // Forward phone->ESP write straight to STM32 UART, append newline.
    const std::string &v = c->getValue();
    if (!v.empty())
    {
      SensorUart.write((const uint8_t *)v.data(), v.size());
      SensorUart.write('\n');
      // Optional: also echo to phone as debug
      bleLogf("DBG: rx->uart %s\n", v.data());
    }
  }
};

class TxCallbacks : public NimBLECharacteristicCallbacks
{
  void onSubscribe(NimBLECharacteristic *, NimBLEConnInfo &, uint16_t subValue) override
  {
    g_notifyEnabled = (subValue & 0x0001);
    bleLogf("DBG: subscribe 0x%04X -> %s\n", subValue, g_notifyEnabled ? "on" : "off");
  }
};

class ServerCallbacks : public NimBLEServerCallbacks
{
  void onConnect(NimBLEServer *, NimBLEConnInfo &) override
  {
    g_hasClient = true;
    bleLogf("DBG: client connected\n");
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
  {
    bleLogf("DBG: client disconnected\n");
    g_hasClient = false;
    g_notifyEnabled = false;
    NimBLEDevice::startAdvertising();
  }
};

// ---------- UART line pump (normalize CRLF, prefix non-GPS with DBG:) ----------
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

      if (rlen == RBUF_MAX - 1)
        bleLogf("DBG: UART overflow; line truncated\n");

      // Decide how to present to phone
      if (is_gps_line(rbuf))
      {
        // Forward GPS line exactly as received
        notify_chunked(rbuf, rlen);
      }
      else
      {
        // Prefix with DBG: and ensure single newline
        // Strip any trailing '\n' to avoid double NL
        size_t payloadLen = rlen;
        if (payloadLen && rbuf[payloadLen - 1] == '\n')
          payloadLen--;
        bleLogf("DBG: %.*s\n", (int)payloadLen, rbuf);
      }

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
  NimBLEDevice::init("ESP32-NUS-1");
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
  sd.setName("ESP32-NUS-1");
  adv->setScanResponseData(sd);
  adv->start();
}

void loop()
{
  pump_uart();
}
