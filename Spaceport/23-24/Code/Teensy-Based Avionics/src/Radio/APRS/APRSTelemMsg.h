#ifndef APRS_TELEM_MSG_H
#define APRS_TELEM_MSG_H

#include "APRSMsgBase.h"
#include <imumaths.h>

const uint8_t PI_ON = 0b00000001;          // Pi is on
const uint8_t PI_VIDEO = 0b00000010;       // Pi is recording video
const uint8_t RECORDING_DATA = 0b00000100; // FC is recording data

struct APRSTelemData final
{
    double lat;
    double lng;
    double alt;
    double spd;
    double hdg;
    int stage;
    imu::Vector<3> orientation;
    uint8_t statusFlags;
};

class APRSTelemMsg final : public APRSMsgBase
{
public:
    APRSTelemMsg(APRSHeader header);
    APRSTelemData data;

private:
    void decodeData(int cursor) override;
    void encodeData(int cursor) override;

    // Scale factors for encoding/decoding ignoring lat/long
    const double ALT_SCALE = (pow(91, 2) / 16000.0);       // (91^2/16000) scale to fit in 2 base91 characters
    const double SPD_SCALE = (pow(91, 2) / 1000.0);        // (91^2/1000) scale to fit in 2 base91 characters
    const double HDG_SCALE = (pow(91, 2) / 360.0);         // (91^2/360) scale to fit in 2 base91 characters
    const double ORIENTATION_SCALE = (pow(91, 2) / 360.0); // same as course

    const int ALT_OFFSET = +1000; // range of -1000 to 15000 ft.
};

#endif // APRS_TELEM_MSG_H