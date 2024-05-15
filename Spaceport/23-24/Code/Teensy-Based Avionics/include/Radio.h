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

/*
Types:
- ENCT_TELEMETRY: latitude,longitude,altitude,speed,heading,precision,stage,t0 <-> APRS message
- ENCT_VIDEO: char* filled with raw bytes <-> Raw byte array
- ENCT_GROUNDSTATION: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value <-> APRS message
- ENCT_NONE: no encoding is applied, same as using tx()
*/
enum EncodingType
{
    ENCT_TELEMETRY,
    ENCT_VIDEO,
    ENCT_GROUNDSTATION,
    ENCT_NONE
};

class RadioMessage
{
public:
    virtual uint8_t *encode() = 0;                            // use stored variables to encode a message
    virtual bool decode(const uint8_t *message, int len) = 0; // use `message` to set stored variables
    virtual int length() const = 0;                           // return the length of the message
    virtual ~RadioMessage(){};

protected:
    uint8_t len;
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
};

#endif // RADIO_H