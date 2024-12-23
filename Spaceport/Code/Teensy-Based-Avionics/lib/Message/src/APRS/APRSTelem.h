#ifndef APRSTELEM_H
#define APRSTELEM_H

#include "APRSData.h"

class APRSTelem : public APRSData
{
public:
    // Scale factors for encoding/decoding ignoring lat/long
    const double ALT_SCALE = (pow(91, 3) / 36000.0);       // (91^3/36000) scale to fit in 3 base91 characters
    const double SPD_SCALE = (pow(91, 2) / 1000.0);        // (91^2/1000) scale to fit in 2 base91 characters
    const double HDG_SCALE = (pow(91, 2) / 360.0);         // (91^2/360) scale to fit in 2 base91 characters
    const double ORIENTATION_SCALE = (pow(91, 2) / 360.0); // same as heading

    const int ALT_OFFSET = +1000; // range of -1000 to 35000 ft.

    double lat = 0.0;                   // decimal latitude
    double lng = 0.0;                   // decimal longitude
    double alt = 0.0;                   // ft
    double spd = 0.0;                   // knots
    double hdg = 0.0;                   // degrees
    double orient[3] = {0.0, 0.0, 0.0}; // euler angles in degrees
    uint32_t stateFlags = 0x00000000;   // flight computer specific state flags (max 32 bits)
                                        // maybe a bit much, but we can fit a lot of info here

    // APRSTelem constructor
    // - config : the APRS config to use
    APRSTelem(APRSConfig config);
    // APRSTelem constructor
    // - config : the APRS config to use
    // - lat : the current decimal latitude (degrees)
    // - lng : the current decimal longitude (degrees)
    // - alt : the current altitude (ft)
    // - spd : the current speed (knots)
    // - hdg : the current heading (degrees)
    // - orient[3] : the current XYZ orientation (degrees)
    // - stateFlags : various data representing the state of the rocket
    APRSTelem(APRSConfig config, double lat, double lng, double alt, double spd, double hdg, double orient[3], uint32_t stateFlags);

    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decode(uint8_t *data, uint16_t sz) override;

    uint16_t toJSON(char *json, uint16_t sz, const char *streamName = "") override;
    uint16_t fromJSON(char *json, uint16_t sz, char *streamName) override;
};

#endif