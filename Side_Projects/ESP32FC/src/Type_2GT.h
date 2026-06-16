// Type2GT LoRa module driver (LR1121 wrapper).
//
// Copied from the flight radio project (Side_Projects/Radio_Code) so both ends
// of the ARC link share the exact same PHY + RF-switch config. Keep the two
// copies in sync (or factor into a shared lib) if you change one.
//
// (The original is guarded by #ifdef STM32; this copy is unconditional because
// it only lives in the ESP32 ground-radio project.)

#ifndef TYPE_2GT_H
#define TYPE_2GT_H

#include <RadioLib.h>

enum RAD_STATE{
    RAD_IDLE,
    RAD_TX,
    RAD_RX,
    RAD_HAS_DATA
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
    RAD_STATE state = RAD_IDLE;
    int lastReceiveRc = RADIOLIB_ERR_NONE;
    uint32_t receiveStartCount = 0;

};

#endif
