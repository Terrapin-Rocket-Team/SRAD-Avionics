#ifndef APRS_MSG_BASE_H
#define APRS_MSG_BASE_H

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

#include "../RadioMessage.h"

struct APRSHeader final
{
    char CALLSIGN[8];
    char TOCALL[8];
    char PATH[10];
    char SYMBOL;
    char OVERLAY;
};

class APRSMsgBase : public RadioMessage
{
public:
    APRSMsgBase(APRSHeader header);
    virtual ~APRSMsgBase(){};

    virtual bool decode() override;
    virtual bool encode() override;
    APRSHeader header;

protected:
    int encodeHeader();
    int decodeHeader();
    virtual void encodeData(int cursor) = 0;
    virtual void decodeData(int cursor) = 0;
    void encodeBase91(uint8_t *message, int &cursor, int value, int precision);
    void decodeBase91(const uint8_t *message, int &cursor, double &value, int precision);

};

#endif // RADIO_H