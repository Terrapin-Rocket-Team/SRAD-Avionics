#ifndef TYPE_2GT_H
#define TYPE_2GT_H

#include <RadioLib.h>

class Type2GT
{
public:
    Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t bsy, SPIClass &spi);
    int begin();
    void onIrq(void (*func)(void));
    int recieve();
    int transmit(const char *str);
    bool hasData();
    void readData(char *str, int len);
    void respondToIrq();

private:
    Module *module;
    RFM96 rad;
    enum State { IDLE, TX, RX, HAS_DATA };
    State state = IDLE;
};

#endif