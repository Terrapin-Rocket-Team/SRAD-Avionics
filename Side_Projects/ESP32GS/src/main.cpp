#include <NimBLEDevice.h>
#include <stdarg.h>

// ===== UART on RX0/TX0 (ESP32-S3: GPIO44/43) =====
static const uint32_t UART_BAUD = 115200;
HardwareSerial SensorUart(0);       // UART0
static const int UART_RX_GPIO = 44; // RX0 pad (module pin 36)
static const int UART_TX_GPIO = 43; // TX0 pad (module pin 37)

// ===== NUS UUIDs =====
static const char *NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *NUS_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write
static const char *NUS_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify

NimBLECharacteristic *pTxChar = nullptr;
volatile bool g_hasClient = false;
volatile bool g_notifyEnabled = false;

// ========== Notify helpers (debug over BLE) ==========
static void notify_chunked(const char *data, size_t len)
{
  if (!g_hasClient || !g_notifyEnabled || !pTxChar || len == 0)
    return;
  const uint16_t mtu = NimBLEDevice::getMTU();          // 23..247
  const size_t maxPayload = (mtu > 3) ? (mtu - 3) : 20; // ATT notif payload
  for (size_t off = 0; off < len; off += maxPayload)
  {
    size_t n = (len - off > maxPayload) ? maxPayload : (len - off);
    pTxChar->setValue((uint8_t *)(data + off), n);
    pTxChar->notify();
  }
}

static void bleLog(const char *s)
{
  notify_chunked(s, strlen(s));
}

static void bleLogf(const char *fmt, ...)
{
  if (!g_hasClient || !g_notifyEnabled)
    return;
  static char buf[200];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n > 0)
    notify_chunked(buf, (size_t)n);
}

// ========== Callbacks ==========
class RxCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &) override
  {
    auto v = c->getValue();
    if (v.length() > 0)
    {
      bleLog("DBG: RX write -> echo\n");
      pTxChar->setValue((uint8_t *)v.data(), v.size());
      pTxChar->notify();
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
    bleLog("DBG: client connected\n");
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
  {
    bleLog("DBG: client disconnected\n");
    g_hasClient = false;
    g_notifyEnabled = false;
    NimBLEDevice::startAdvertising();
  }
};

// ---- CSV parser: extract fields 15,16,17 (1-indexed) -> lat,lon,alt
static bool parse_lla_from_csv(const char *s, double &lat, double &lon, double &alt)
{
  int field = 1;
  const char *p = s, *start = s;
  bool gotLat = false, gotLon = false, gotAlt = false;

  while (true)
  {
    char c = *p;
    if (c == ',' || c == '\0' || c == '\n' || c == '\r')
    {
      if (field == 15)
      {
        lat = strtod(start, nullptr);
        gotLat = true;
      }
      else if (field == 16)
      {
        lon = strtod(start, nullptr);
        gotLon = true;
      }
      else if (field == 17)
      {
        alt = strtod(start, nullptr);
        gotAlt = true;
      }
      if (c == '\0' || c == '\n' || c == '\r')
        break;
      field++;
      start = p + 1;
    }
    if (c == '\0')
      break;
    ++p;
  }
  return gotLat && gotLon && gotAlt;
}

// ---- UART ring buffer (handles long lines)
static const size_t RBUF_MAX = 1024;
static char rbuf[RBUF_MAX];
static size_t rlen = 0;

static void pump_uart()
{
  while (SensorUart.available())
  {
    char c = (char)SensorUart.read();
    if (c == '\r')
      continue; // normalize CRLF
    if (rlen < RBUF_MAX - 1)
      rbuf[rlen++] = c;
    if (c == '\n' || rlen == RBUF_MAX - 1)
    { // got a line or overflow
      rbuf[rlen] = '\0';

      if (rlen == RBUF_MAX - 1)
        bleLog("DBG: UART overflow; line truncated\n");

      double lat = 0, lon = 0, alt = 0;
      if (parse_lla_from_csv(rbuf, lat, lon, alt))
      {
        if (!(lat == 0.0 && lon == 0.0))
        {
          // Data out (this is what Flutter consumes)
          char a[24], b[24], cbuf[24], out[80];
          dtostrf(lat, 0, 7, a);
          while (*a == ' ')
            memmove(a, a + 1, strlen(a));
          dtostrf(lon, 0, 7, b);
          while (*b == ' ')
            memmove(b, b + 1, strlen(b));
          dtostrf(alt, 0, 2, cbuf);
          while (*cbuf == ' ')
            memmove(cbuf, cbuf + 1, strlen(cbuf));
          int n = snprintf(out, sizeof(out), "%s,%s,%s\n", a, b, cbuf);
          if (n > 0)
            notify_chunked(out, (size_t)n);

          // Debug line (comment out if too chatty)
          // bleLogf("DBG: LLA %s,%s,%s\n", a, b, cbuf);
        }
        else
        {
          bleLog("DBG: no-fix (0,0) -> skipped\n");
        }
      }
      else
      {
        bleLogf("DBG: parse miss (len=%u)\n", (unsigned)rlen);
      }
      rlen = 0;
    }
  }
}

void setup()
{
  // UART on RX0/TX0 (GPIO44/43). TX is unused but must be valid.
  SensorUart.begin(UART_BAUD, SERIAL_8N1, UART_RX_GPIO, UART_TX_GPIO);

  NimBLEDevice::init("ESP32-NUS-1");
  NimBLEDevice::setPower(7);
  NimBLEDevice::setMTU(247);

  auto *server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  auto *svc = server->createService(NUS_SERVICE_UUID);

  pTxChar = svc->createCharacteristic(NUS_TX_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pTxChar->setCallbacks(new TxCallbacks());

  auto *rx = svc->createCharacteristic(NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
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
