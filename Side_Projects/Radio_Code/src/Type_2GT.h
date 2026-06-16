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
    int transmit(const char *str);
    int transmit(const uint8_t *data, size_t len);
    bool hasData();
    size_t getPacketLength();
    int readData(uint8_t *data, size_t len);
    void respondToIrq();
    // Retune the LoRa carrier (MHz). Both link ends must agree, so this is
    // driven by the ARC RADIO SET_FREQUENCY flow, not called directly.
    int setFrequency(float freqMHz);
    int applyPhyProfile(uint8_t profileId);
    float getRSSI();
    float getSNR();
    int getLastReceiveRc() const;
    uint32_t getReceiveStartCount() const;
    int getState() const;

private:
    LR1121 rad;
    RAD_STATE state = IDLE;
    int lastReceiveRc = RADIOLIB_ERR_NONE;
    uint32_t receiveStartCount = 0;

};

#endif
#endif
