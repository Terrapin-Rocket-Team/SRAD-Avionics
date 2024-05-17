#ifndef RADIO_H
#define RADIO_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) // Windows
#include <cstdint>
#include <string>
#include <cstring>
#elif defined(__unix__)  // Linux
// TODO
#elif defined(__APPLE__) // OSX
// TODO
#endif

#include "RadioMessage.h"
#include "RadioHead.h"
#include "RHGenericSPI.h"
/*
Settings:
- double frequency in Mhz
- RHGenericSPI *spi pointer to radiohead SPI object
- uint8_t cs CS pin
- uint8_t irq IRQ pin
- uint8_t rst RST pin
*/
struct RadioSettings
{
    double frequency;
    uint8_t thisAddr;
    uint8_t toAddr;
    RHGenericSPI *spi;
    uint8_t cs;
    uint8_t irq;
    uint8_t rst;
    int txPower = 20;
};

class Radio
{
public:
    virtual ~Radio(){}; // Virtual descructor. Very important
    virtual bool init() = 0;
    virtual bool tx(const uint8_t *message, int len = -1, int packetNum = 0, bool lastPacket = true) = 0; // designed to be used internally. cannot exceed 66 bytes including headers
    virtual bool rx(uint8_t *recvbuf, uint8_t *len) = 0;
    virtual int RSSI() = 0;
    virtual bool enqueueSend(const uint8_t *message, uint8_t len) = 0;
    virtual bool enqueueSend(const char *message) = 0;
    virtual bool enqueueSend(RadioMessage *message) = 0;
    virtual bool dequeueReceive(RadioMessage *message) = 0;
    virtual bool dequeueReceive(char *message) = 0;
    virtual bool dequeueReceive(uint8_t *message) = 0;
    virtual bool update() = 0;

protected:
    RadioSettings settings;
};

#endif // RADIO_H