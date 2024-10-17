#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include "RadioMessage.h"

class Radio
{
public:
    virtual ~Radio() {}; // Virtual descructor. Very important
    virtual bool begin() = 0;
    virtual bool tx(const uint8_t *message, int len = -1) = 0;
    virtual bool rx() = 0;
    virtual bool send(Data &data) = 0;
    virtual bool receive(Data &data) = 0;
    virtual int RSSI() = 0;
};

#endif // RADIO_H