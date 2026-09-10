#include "DataRadioTelem.h"

DataRadioTelem::DataRadioTelem(const APRSConfig &config)
    : config_(config) {}

uint16_t DataRadioTelem::build(double lat, double lng, double altFt, double spdKnots,
                               double hdgDeg, const double orient[3],
                               uint8_t tempC, uint8_t stage, uint8_t fixQual)
{
    // APRSTelem takes a non-const orient[3]; copy into a local.
    double orientLocal[3] = {orient[0], orient[1], orient[2]};

    APRSTelem telem(config_, lat, lng, altFt, spdKnots, hdgDeg, orientLocal, 0);

    // Pack auxiliary state into the 32-bit state-flags field: 7 bits temp,
    // 4 bits stage, 4 bits fix quality (same pattern as the legacy avionics).
    uint8_t encoding[] = {7, 4, 4};
    telem.stateFlags.setEncoding(encoding, 3);
    uint8_t packed[] = {tempC, stage, fixQual};
    telem.stateFlags.pack(packed);

    msg_.clear();
    msg_.encode(&telem);
    return msg_.size;
}
