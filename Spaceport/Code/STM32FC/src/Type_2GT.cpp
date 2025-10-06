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
    Serial.printf("DBG: RadioLib begin -> %d\n", rc);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    rad.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    rad.setRegulatorDCDC();

    // >>>> PICK YOUR NETWORK PARAMS <<<<
    // Example: US 915 MHz, SF7, BW125, CR 4/5, sync 0x12, power 14 dBm
    // Change these to match your other node(s).
    rc = rad.setFrequency(915.0);
    Serial.printf("DBG: setFrequency -> %d\n", rc);
    rc = rad.setSpreadingFactor(7);
    Serial.printf("DBG: setSF -> %d\n", rc);
    rc = rad.setBandwidth(125.0);
    Serial.printf("DBG: setBW -> %d\n", rc);
    rc = rad.setCodingRate(5);
    Serial.printf("DBG: setCR -> %d\n", rc);
    rc = rad.setSyncWord(0x12);
    Serial.printf("DBG: setSync -> %d\n", rc);
    rc = rad.setOutputPower(14);
    Serial.printf("DBG: setPower -> %d\n", rc);

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
    if (!rad.available())
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