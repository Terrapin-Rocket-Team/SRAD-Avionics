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

Type2GT::Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t bsy, SPIClass &spi) : rad(new Module(cs, irq, rst, bsy, spi)) {
    
}

int Type2GT::begin()
{
    int i = rad.begin();
    if (!i == RADIOLIB_ERR_NONE)
        return i;
    rad.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
    rad.setRegulatorDCDC();
    return i;
}

void Type2GT::onIrq(void (*func)(void))
{
    rad.setIrqAction(func);
}
int Type2GT::recieve()
{
    state = RX;
    return rad.startReceive();
}
bool Type2GT::transmit(const char *str)
{
    state = TX;
    return rad.startTransmit(str);
}
bool Type2GT::hasData()
{
    return state == HAS_DATA;
}
void Type2GT::readData(char *str, int len)
{
    rad.readData((uint8_t *)str, len);
    if (!rad.available())
        state = IDLE;
}
void Type2GT::respondToIrq()
{
    if (state == TX)
        recieve();
    else if (state == RX){
        if (rad.available())
            state = HAS_DATA;
        else
            state = IDLE;
    }

}