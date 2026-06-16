#include <Arduino.h>

#include "Type_2GT.h"

SPIClass spi(HSPI);
Type2GT radio(/*CS*/ 14, /*DIO1*/ 36, /*RST*/ 38, /*BUSY*/ 37, spi);

volatile bool g_irqFlag = false;

static void IRAM_ATTR onLoraIrq()
{
    g_irqFlag = true;
}

void setup()
{
    USBSerial.begin(115200);
    delay(2500);
    USBSerial.println("\n=== Type2GT wrapper ping TX ===");

    spi.begin(/*sck*/ 13, /*miso*/ 12, /*mosi*/ 11, /*ss*/ 14);
    USBSerial.printf("[WRAP-TX] begin -> %d\n", radio.begin());
    radio.onIrq(onLoraIrq);
    USBSerial.printf("[WRAP-TX] startReceive -> %d\n", radio.recieve());
}

void loop()
{
    if (g_irqFlag)
    {
        g_irqFlag = false;
        radio.respondToIrq();
        if (radio.hasData())
        {
            uint8_t scratch[255];
            const size_t n = radio.getPacketLength();
            radio.readData(scratch, n);
            radio.recieve();
        }
    }

    static uint32_t lastTx = 0;
    static uint32_t seq = 0;
    const uint32_t now = millis();
    if (now - lastTx < 1000)
    {
        return;
    }
    lastTx = now;

    char msg[32];
    snprintf(msg, sizeof(msg), "WRAP/%lu", (unsigned long)seq++);
    const int rc = radio.transmit(reinterpret_cast<const uint8_t *>(msg), strlen(msg));
    USBSerial.printf("[WRAP-TX] tx rc=%d data='%s'\n", rc, msg);
    g_irqFlag = false;
    USBSerial.printf("[WRAP-TX] restart RX -> %d\n", radio.recieve());
}
