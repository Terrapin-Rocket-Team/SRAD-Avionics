#ifndef DATA_RADIO_TELEM_H
#define DATA_RADIO_TELEM_H

#include <Arduino.h>

#include "RadioMessage.h"

// Builder for the "other" telemetry system's downlink frame.
//
// Produces a RadioMessage APRSTelem frame (the format the Terrapin ground
// station already decodes) into an internal buffer. The bytes are opaque to
// ARC: they are wrapped in a RADIO/DATA_DOWNLINK frame and routed to the
// proprietary data radio, which transmits them verbatim. Nothing here touches
// the FC's onboard radio. Payload fields are placeholders for now.
class DataRadioTelem
{
public:
    explicit DataRadioTelem(const APRSConfig &config);

    // Encode one telemetry frame into the internal buffer.
    // - lat/lng:  decimal degrees
    // - altFt:    altitude AGL (ft)
    // - spdKnots: speed (knots)
    // - hdgDeg:   heading (deg)
    // - orient:   XYZ orientation, degrees
    // - tempC/stage/fixQual: packed into the APRS state-flags field
    // Returns the encoded length in bytes (0 on failure).
    uint16_t build(double lat, double lng, double altFt, double spdKnots,
                   double hdgDeg, const double orient[3],
                   uint8_t tempC, uint8_t stage, uint8_t fixQual);

    const uint8_t *data() const { return msg_.buf; }
    uint16_t size() const { return msg_.size; }

private:
    APRSConfig config_;
    Message msg_;
};

#endif // DATA_RADIO_TELEM_H
