#include "Type_2GT.h"
#include <RadioLib.h>

Type2GT::Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t bsy, SPIClass &spi)
    : module(new Module(cs, irq, rst, RADIOLIB_NC, spi)), rad(module) {}

int Type2GT::begin()
{
    int rc = rad.begin();
    Serial.printf("DBG: RadioLib begin -> %d\n", rc);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    // LoRa PHY settings to match your network
    rc = rad.setFrequency(915.0);
    Serial.printf("DBG: setFrequency -> %d\n", rc);
    
    rc = rad.setBandwidth(125.0);
    Serial.printf("DBG: setBW -> %d\n", rc);
    
    rc = rad.setSpreadingFactor(7);
    Serial.printf("DBG: setSF -> %d\n", rc);
    
    rc = rad.setCodingRate(5);
    Serial.printf("DBG: setCR -> %d\n", rc);
    
    rc = rad.setPreambleLength(8);
    Serial.printf("DBG: setPreamble -> %d\n", rc);
    
    rc = rad.setCRC(true);
    Serial.printf("DBG: setCRC -> %d\n", rc);
    
    rc = rad.setSyncWord(0x34);
    Serial.printf("DBG: setSync -> %d\n", rc);
    
    rc = rad.setOutputPower(14);
    Serial.printf("DBG: setPower -> %d\n", rc);
    
    rad.explicitHeader();
    rad.invertIQ(false);
    
    return RADIOLIB_ERR_NONE;
}

void Type2GT::onIrq(void (*func)(void))
{
    rad.setDio0Action(func, RISING);
}

int Type2GT::recieve()
{
    state = RX;
    int rc = rad.startReceive();
    Serial.printf("DBG: startReceive -> %d\n", rc);
    return rc;
}

int Type2GT::transmit(const char *str)
{
    state = TX;
    const size_t len = strlen(str);
    Serial.printf("DBG: startTransmit len=%u: \"%.40s%s\"\n",
                  (unsigned)len, str, (len > 40 ? "..." : ""));
    int rc = rad.startTransmit(str);
    Serial.printf("DBG: startTransmit -> %d\n", rc);
    return rc;
}

bool Type2GT::hasData()
{
    return state == HAS_DATA;
}

void Type2GT::readData(char *str, int len)
{
    int n = rad.readData((uint8_t *)str, len);
    Serial.printf("DBG: readData -> %d\n", n);
    if (n < 0 || !rad.available())
        state = IDLE;
}

void Type2GT::respondToIrq()
{
    if (state == TX)
    {
        Serial.println("DBG: IRQ after TX -> switch to RX");
        recieve();
    }
    else if (state == RX)
    {
        const bool avail = rad.available();
        Serial.printf("DBG: IRQ RX -> available=%d\n", (int)avail);
        state = avail ? HAS_DATA : IDLE;
    }
}