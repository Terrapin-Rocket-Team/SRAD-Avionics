#ifdef STM32
#include "Type_2GT.h"
#include "arc_messages_radio.h"

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

static int applyCommonLoRaSettings(LR1121 &rad, float bandwidthKHz)
{
    int rc = rad.setSpreadingFactor(7);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;
    rc = rad.setBandwidth(bandwidthKHz);
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
    return rad.setOutputPower(14);
}

int Type2GT::begin()
{
    int rc = rad.begin();
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    rad.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    rad.setRegulatorDCDC();

    // Keep PHY exactly aligned with the ESP32 receiver side. Boot on the shared
    // home channel (see kDefaultFreqMHz in main.cpp); both ends must agree.
    rc = rad.setFrequency(909.5);
    if (rc != RADIOLIB_ERR_NONE)
        return rc;

    return applyPhyProfile(ARC_RADIO_PHY_PROFILE_SAFE_BW125);
}

void Type2GT::onIrq(void (*func)(void))
{
    rad.setIrqAction(func);
}

int Type2GT::recieve()
{
    receiveStartCount++;
    int rc = rad.explicitHeader();
    if (rc != RADIOLIB_ERR_NONE)
    {
        lastReceiveRc = rc;
        state = IDLE;
        return lastReceiveRc;
    }
    lastReceiveRc = rad.startReceive();
    state = (lastReceiveRc == RADIOLIB_ERR_NONE) ? RX : IDLE;
    return lastReceiveRc;
}

int Type2GT::transmit(const char *str)
{
    // Use blocking TX to guarantee packet completion before next send.
    state = TX;
    int rc = rad.transmit(str);
    state = IDLE;
    return rc;
}

int Type2GT::transmit(const uint8_t *data, size_t len)
{
    state = TX;
    int rc = rad.transmit(data, len);
    state = IDLE;
    return rc;
}

bool Type2GT::hasData()
{
    return state == HAS_DATA;
}

size_t Type2GT::getPacketLength()
{
    return rad.getPacketLength();
}

int Type2GT::readData(uint8_t *data, size_t len)
{
    const int rc = rad.readData(data, len);
    state = IDLE;
    return rc;
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
        state = HAS_DATA;
    }
}

int Type2GT::setFrequency(float freqMHz)
{
    return rad.setFrequency(freqMHz);
}

int Type2GT::applyPhyProfile(uint8_t profileId)
{
    switch (profileId)
    {
    case ARC_RADIO_PHY_PROFILE_SAFE_BW125:
        return applyCommonLoRaSettings(rad, 125.0f);
    case ARC_RADIO_PHY_PROFILE_FAST_BW500:
        return applyCommonLoRaSettings(rad, 500.0f);
    default:
        return RADIOLIB_ERR_INVALID_BANDWIDTH;
    }
}

float Type2GT::getRSSI()
{
    return rad.getRSSI();
}

float Type2GT::getSNR()
{
    return rad.getSNR();
}

int Type2GT::getLastReceiveRc() const
{
    return lastReceiveRc;
}

uint32_t Type2GT::getReceiveStartCount() const
{
    return receiveStartCount;
}

int Type2GT::getState() const
{
    return static_cast<int>(state);
}
#endif
