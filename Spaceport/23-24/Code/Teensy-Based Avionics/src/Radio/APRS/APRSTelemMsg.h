#ifndef APRS_TELEM_MSG_H
#define APRS_TELEM_MSG_H

#include "APRSMsgBase.h"



struct APRSTelemData
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

class APRSTelemMsg : public APRSMsgBase
{
public:
    APRSTelemData data;
    APRSTelemMsg(APRSHeader &header);

protected:
    virtual void decodeData(int cursor) override;
    virtual void encodeData(int cursor) override;
private:
    void encodeStatus(uint8_t *string, int &cursor);
    // Scale factors for encoding/decoding ignoring lat/long
    const double ALT_SCALE = (pow(91, 2) / 16000.0);       // (91^2/16000) scale to fit in 2 base91 characters
    const double SPD_SCALE = (pow(91, 2) / 1000.0);        // (91^2/1000) scale to fit in 2 base91 characters
    const double HDG_SCALE = (pow(91, 2) / 360.0);         // (91^2/360) scale to fit in 2 base91 characters
    const double ORIENTATION_SCALE = (pow(91, 2) / 360.0); // same as course

    const int ALT_OFFSET = +1000; // range of -1000 to 15000 ft.
};

#endif // APRS_TELEM_MSG_H