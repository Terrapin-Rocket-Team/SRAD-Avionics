#ifdef STM32
#include "Type_2GT.h"

static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
    RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                  DIO5  DIO6  DIO7
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

Type2GT::Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t bsy, SPIClass &spi)
    : rad(new Module(cs, irq, rst, bsy, spi)) {}

int Type2GT::begin()
{
    int rc = rad.begin();
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    rad.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    rad.setRegulatorDCDC();

    // Keep PHY exactly aligned with the ESP32 receiver side.
    rc = rad.setFrequency(915.0);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setSpreadingFactor(7);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setBandwidth(125.0);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setCodingRate(5);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setSyncWord(0x12);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setPreambleLength(8);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setCRC(true);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.explicitHeader();
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.invertIQ(false);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setOutputPower(14);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    return rc;
}

void Type2GT::onIrq(void (*func)(void))
{
    rad.setIrqAction(func);
}

int Type2GT::recieve()
{
    state = RX;
    int rc = rad.startReceive();
    //Serial.printf("DBG: startReceive -> %d\n", rc);
    return rc;
}

int Type2GT::transmit(const char *str)
{
    // Use blocking TX to guarantee packet completion before next send.
    state = TX;
    int rc = rad.transmit(str);
    state = IDLE;
    return rc;
}

bool Type2GT::hasData()
{
    return state == HAS_DATA;
}

void Type2GT::readData(char *str, int len)
{
    int n = rad.readData((uint8_t *)str, len);
    //Serial.printf("DBG: readData -> %d\n", n);
    if (!rad.available())
        state = IDLE;
}

void Type2GT::respondToIrq()
{
    if (state == TX)
    {
        // TX complete IRQ.
        state = IDLE;
    }
    else if (state == RX)
    {
        const bool avail = rad.available();
        //Serial.printf("DBG: IRQ RX -> available=%d\n", (int)avail);
        state = avail ? HAS_DATA : IDLE;
    }
}
#endif
