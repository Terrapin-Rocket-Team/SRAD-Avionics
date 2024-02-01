#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>

/*
Types:
0 - Telemetry
1 - Video
*/

class Radio
{
public:
    virtual ~Radio(){}; // Virtual descructor. Very important
    virtual void begin() = 0;
    virtual bool tx(String message) = 0;
    virtual String rx() = 0;
    virtual bool encode(String &message, int type) = 0;
    virtual bool decode(String &message, int type) = 0;
    virtual bool send(String message, int type) = 0;
    virtual String receive(int type) = 0;
    virtual int RSSI() = 0;
};

struct APRSConfig
{
    char *CALLSIGN;
    char *TOCALL;
    char *PATH;
    char *SYMBOL;
    char *OVERLAY;
};

#endif // RADIO_H