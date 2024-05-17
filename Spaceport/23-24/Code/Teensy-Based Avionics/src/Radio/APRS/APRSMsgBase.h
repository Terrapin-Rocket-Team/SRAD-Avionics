#ifndef APRSMSG_H
#define APRSMSG_H

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

#include "../Radio.h"
#include <imumaths.h>



/*
APRS Configuration
- CALLSIGN
- TOCALL
- PATH
- SYMBOL
- OVERLAY
*/
struct APRSHeader
{
    char CALLSIGN[8];
    char TOCALL[8];
    char PATH[10];
    char SYMBOL;
    char OVERLAY;
};

/*
APRS Telemetry Data
- lat
- lng
- alt
- spd
- hdg
- stage
- orientation
*/


class APRSMsgBase : public RadioMessage
{
public:
    APRSMsgBase(APRSHeader &header);
    virtual ~APRSMsgBase(){};

    virtual bool decode() override;
    virtual bool encode() override;
    APRSHeader header;
protected:
    virtual int encodeHeader();
    virtual int decodeHeader();
    virtual void encodeData(int cursor) = 0;
    virtual void decodeData(int cursor) = 0;
    void encodeBase91(uint8_t *message, int &cursor, int value, int precision);
    void decodeBase91(const uint8_t *message, int &cursor, double &value, int precision);

private:
};

#endif // RADIO_H