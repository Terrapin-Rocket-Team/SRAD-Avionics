#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>

class Radio
{
public:
    virtual ~Radio(){}; // Virtual descructor. Very important
    virtual void begin() = 0;
    virtual bool tx(String message) = 0;
    virtual String rx() = 0;
    virtual bool encode(String &message, int type) = 0;
    virtual bool decode(String &message, int type) = 0;
};

#endif // RADIO_H