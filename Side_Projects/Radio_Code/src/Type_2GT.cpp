#ifdef STM32
#include "Type_2GT.h"
#include <string.h>

static const uint8_t EVENT_MASK_TX_DONE = 0x01;
static const uint8_t EVENT_MASK_RX_DONE = 0x02;

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
    rc = rad.setBandwidth(250.0);
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
    if (rc != RADIOLIB_ERR_NONE)
        state = IDLE;
    // Serial.printf("DBG: startReceive -> %d\n", rc);
    return rc;
}

int Type2GT::transmit(const char *str)
{
    if (state == TX)
    {
        return TYPE2GT_ERR_BUSY;
    }

    state = TX;
    txStartUs = micros();
    int rc = rad.startTransmit(str);
    if (rc != RADIOLIB_ERR_NONE)
        state = IDLE;

    // Serial.printf("DBG: startTransmit -> %d\n", rc);
    return rc;
}

bool Type2GT::hasData()
{
    return state == HAS_DATA;
}

int Type2GT::readData(char *str, int len)
{
    if (!str || len < 2)
        return TYPE2GT_ERR_BAD_ARGS;

    memset(str, 0, len);
    int rc = rad.readData((uint8_t *)str, (size_t)(len - 1));
    if (rc != RADIOLIB_ERR_NONE)
    {
        state = IDLE;
        return rc;
    }

    str[len - 1] = '\0';
    state = IDLE;
    return (int)strnlen(str, (size_t)(len - 1));
}

bool Type2GT::popEvent(RAD_EVENT &event)
{
    noInterrupts();
    const uint8_t events = pendingEvents;

    if ((events & EVENT_MASK_TX_DONE) != 0)
    {
        pendingEvents = (uint8_t)(pendingEvents & ~EVENT_MASK_TX_DONE);
        interrupts();
        event = RAD_EVENT_TX_DONE;
        return true;
    }

    if ((events & EVENT_MASK_RX_DONE) != 0)
    {
        pendingEvents = (uint8_t)(pendingEvents & ~EVENT_MASK_RX_DONE);
        interrupts();
        event = RAD_EVENT_RX_DONE;
        return true;
    }

    interrupts();
    event = RAD_EVENT_NONE;
    return false;
}

void Type2GT::handleIrq()
{
    if (state == TX)
    {
        txIrqUs = micros();
        pendingEvents = (uint8_t)(pendingEvents | EVENT_MASK_TX_DONE);
        state = IDLE;
    }
    else if (state == RX || state == HAS_DATA)
    {
        pendingEvents = (uint8_t)(pendingEvents | EVENT_MASK_RX_DONE);
        state = HAS_DATA;
    }
}

bool Type2GT::isBusy() const
{
    return state == TX;
}

uint32_t Type2GT::lastTxDurationUs() const
{
    noInterrupts();
    const uint32_t start = txStartUs;
    const uint32_t done = txIrqUs;
    interrupts();
    return (uint32_t)(done - start);
}

void Type2GT::respondToIrq()
{
    handleIrq();
}
#endif
