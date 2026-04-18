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

    // >>>> PICK YOUR NETWORK PARAMS <<<<
    // Example: US 915 MHz, SF7, BW125, CR 4/5, sync 0x12, power 14 dBm
    // Change these to match your other node(s).
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

    rc = rad.setOutputPower(14);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    return RADIOLIB_ERR_NONE;
}

void Type2GT::onIrq(void (*func)(void))
{
    rad.setIrqAction(func);
}

int Type2GT::recieve()
{
    state = RX;
    int rc = rad.startReceive();
    if (rc != RADIOLIB_ERR_NONE)
        state = IDLE;
    return rc;
}

int Type2GT::transmit(const char *str)
{
    state = TX;
    const size_t len = strlen(str);
    int rc = rad.startTransmit(str);
    if (rc != RADIOLIB_ERR_NONE)
        state = IDLE;
    return rc;
}

int Type2GT::transmit(const uint8_t *data, size_t len)
{
    state = TX;
    int rc = rad.startTransmit(data, len);
    if (rc != RADIOLIB_ERR_NONE)
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
    (void)n;
    state = IDLE;
}

void Type2GT::respondToIrq()
{
    irqPending = true;
}

void Type2GT::service()
{
    if (!irqPending)
        return;

    irqPending = false;

    if (state == TX)
    {
        // After TX_DONE, return to RX in thread context instead of inside the ISR.
        recieve();
    }
    else if (state == RX)
    {
        // DIO1 is only mapped to RX_DONE in packet mode, so a pending IRQ means
        // there is a packet ready to be read.
        state = HAS_DATA;
    }
}
#endif
