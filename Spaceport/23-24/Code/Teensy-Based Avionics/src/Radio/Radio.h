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
#include "RHGenericSPI.h" //PIO doesn't like RadioHead without this

struct RadioSettings final
{
    double frequency; // in Mhz
    uint8_t thisAddr; // RadioHead address of this device
    uint8_t toAddr;  // RadioHead address of the device to send to
    RHGenericSPI *spi; // RadioHead SPI object
    uint8_t cs; // Chip Select pin
    uint8_t irq; // Interrupt Request pin
    uint8_t rst; // Reset pin
    int txPower = 20; // in dBm
};

class Radio
{
public:
    virtual ~Radio(){};
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

    virtual explicit operator bool() const { return initialized; }
    virtual bool update() = 0; // should be called in the main loop

protected:
    RadioSettings settings;
    bool initialized = false;
};

#endif // RADIO_H