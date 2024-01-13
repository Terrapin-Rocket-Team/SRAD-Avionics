#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>

enum EncodingType
{
    ENCT_TELEMETRY,
    ENCT_VIDEO,
    ENCT_GROUNDSTATION
};

class Radio
{
public:
    virtual ~Radio(){}; // Virtual descructor. Very important
    virtual void begin() = 0;
    virtual bool tx(String message) = 0;
    virtual String rx() = 0;
    virtual bool encode(String &message, EncodingType type) = 0;
    virtual bool decode(String &message, EncodingType type) = 0;
    virtual bool send(String message, EncodingType type) = 0;
    virtual String receive(EncodingType type) = 0;
    virtual int RSSI() = 0;
};

struct APRSConfig
{
    char CALLSIGN[8];
    char TOCALL[8];
    char PATH[10];
    char SYMBOL;
    char OVERLAY;
};

struct APRSData
{
    char lat[16];
    char lng[16];
    char alt[10];
    char spd[4];
    char hdg[4];
    char precision;
    char stage[3];
    char t0[9];
    char dao[6];
};

#endif // RADIO_H