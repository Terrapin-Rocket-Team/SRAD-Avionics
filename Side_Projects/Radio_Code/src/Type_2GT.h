#ifdef STM32
#ifndef TYPE_2GT_H
#define TYPE_2GT_H

#include <RadioLib.h>

static const int TYPE2GT_ERR_BUSY = -7001;
static const int TYPE2GT_ERR_BAD_ARGS = -7002;

enum RAD_STATE{
    IDLE,
    TX,
    RX,
    HAS_DATA
};

enum RAD_EVENT {
    RAD_EVENT_NONE = 0,
    RAD_EVENT_TX_DONE = 1,
    RAD_EVENT_RX_DONE = 2,
};

class Type2GT{
public:
    Type2GT(uint8_t cs, uint8_t irq, uint8_t rst, uint8_t busy, SPIClass &spi);
    int begin();
    void onIrq(void (*func)(void));
    int recieve();
    int transmit(const char *str); // was bool
    bool hasData();
    int readData(char *str, int len);
    bool popEvent(RAD_EVENT &event);
    void handleIrq();
    bool isBusy() const;
    uint32_t lastTxDurationUs() const;
    void respondToIrq();

private:
    LR1121 rad;
    volatile RAD_STATE state = IDLE;
    volatile uint8_t pendingEvents = 0;
    volatile uint32_t txStartUs = 0;
    volatile uint32_t txIrqUs = 0;

};

#endif
#endif
