#ifdef STM32
#ifndef TYPE_2GT_H
#define TYPE_2GT_H

#include <RadioLib.h>

enum RAD_STATE{
    IDLE,
    TX,
    RX,
    HAS_DATA
};

class Type2GT{
public:
    Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t busy, SPIClass &spi);
    int begin();
    void onIrq(void (*func)(void));
    int recieve();
    int transmit(const char *str); // was bool
    bool hasData();
    void readData(char *str, int len);
    void respondToIrq();

private:
    LR1121 rad;
    RAD_STATE state = IDLE;

};

#endif
#endif