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

#include "Radio.h"
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
};

/*
APRS Telemetry Data
- lat
- lng
- alt
- spd
- hdg
- orientation
- stage
*/
struct APRSData
{
    double lat;
    double lng;
    double alt;
    double spd;
    double hdg;
    int stage;
    imu::Vector<3> orientation;
};

class APRSMsg : public RadioMessage
{
public:
    APRSMsg(APRSHeader &header);
    virtual ~APRSMsg(){};

    bool decode(const uint8_t *message, int len) override;
    bool encode(uint8_t *message) override;
    int length() const override { return this->len; }
    APRSData data;
    APRSHeader header;

private:
    int encodeHeader(uint8_t *message) const;
    void encodeData(uint8_t *message, int cursor);

    int decodeHeader(const uint8_t *message, int len);
    void decodeData(const uint8_t *message, int len, int cursor);

    void encodeBase91(uint8_t *message, int &cursor, int value, int precision) const;
    void decodeBase91(const uint8_t *message, int &cursor, double &value, int precision) const;

    // Scale factors for encoding/decoding ignoring lat/long
    const double ALT_SCALE = (pow(91, 2) / 16000.0);       // (91^2/16000) scale to fit in 2 base91 characters
    const double SPD_SCALE = (pow(91, 2) / 1000.0);        // (91^2/1000) scale to fit in 2 base91 characters
    const double HDG_SCALE = (pow(91, 2) / 360.0);         // (91^2/360) scale to fit in 2 base91 characters
    const double ORIENTATION_SCALE = (pow(91, 2) / 360.0); // same as course

    const int ALT_OFFSET = +1000; // range of -1000 to 15000 ft.
};

#endif // RADIO_H