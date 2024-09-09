#include "APRSTelem.h"

APRSTelem::APRSTelem(APRSConfig config) : APRSData(config)
{
}

APRSTelem::APRSTelem(APRSConfig config, double lat, double lng, double alt, double spd, double hdg, double orient[3], uint32_t stateFlags)
    : APRSData(config), lat(lat), lng(lng), alt(alt), spd(spd), hdg(hdg), stateFlags(stateFlags)
{
    this->orient[0] = orient[0];
    this->orient[1] = orient[1];
    this->orient[2] = orient[2];
}

uint16_t APRSTelem::encode(uint8_t *data, uint16_t sz)
{
    uint16_t pos = 0;

    // APRS header
    this->encodeHeader(data, sz, pos);

    // telemetry ("!MYYYYXXXX^ hhssaaoooooofffff")
    // ! = message type     hh = heading
    // M = overlay          ss = speed
    // YYYY = latitude      aa = altitude
    // XXXX = longitude     oooooo = orient
    // ^ = symbol           fffff = state flags

    // lat/long
    if (sz < pos + 11)
        return 0; // error too small for lat/long

    data[pos++] = this->config.type;
    data[pos++] = this->config.overlay;
    base10toBase91(data, pos, 380926 * (90 - this->lat), 4);  // magic formula from the APRS spec (page 38)
    base10toBase91(data, pos, 190463 * (180 + this->lng), 4); // magic formula from the APRS spec (page 38)
    data[pos++] = this->config.symbol;
    data[pos++] = ' '; // space to indicate start of comment section

    // heading, speed, alt
    if (sz < pos + 2 + 2 + 3)
        return 0; // error too small for heading, speed, and alt

    base10toBase91(data, pos, (int)(this->hdg * HDG_SCALE), 2);                // (91^2/360) scale to fit in 2 base91 characters
    base10toBase91(data, pos, (int)(this->spd * SPD_SCALE), 2);                // (91^2/1000) scale to fit in 2 base91 characters. 1000 knots is the assumed max speed.
    base10toBase91(data, pos, (int)((this->alt + ALT_OFFSET) * ALT_SCALE), 3); // (91^3/35000) scale to fit in 3 base91 characters. 35000 feet is the assumed max altitude.

    // orientation
    if (sz < pos + 2 + 2 + 2)
        return 0; // error too small for orientation

    base10toBase91(data, pos, (int)(this->orient[0] * ORIENTATION_SCALE), 2); // (91^2/360) scale to fit in 2 base91 characters
    base10toBase91(data, pos, (int)(this->orient[1] * ORIENTATION_SCALE), 2);
    base10toBase91(data, pos, (int)(this->orient[2] * ORIENTATION_SCALE), 2);

    // state info
    if (sz < pos + 5)
        return 0; // error too small for state flags

    // max value is 4294967295, so we need 5 base 91 (or 8 hex) digits to represent it
    base10toBase91(data, pos, this->stateFlags >> 19, 2);               // shift off everything but the first 13 bits
    base10toBase91(data, pos, (this->stateFlags & 0x0007FFC0) >> 6, 2); // shift off and remove everything but the next 13 bits
    base10toBase91(data, pos, this->stateFlags & 0x000003F, 1);         // get the last 6 bits

    return pos;
}

uint16_t APRSTelem::decode(uint8_t *data, uint16_t sz)
{
    uint16_t pos = 0;
    uint16_t decodedNum = 0;

    this->decodeHeader(data, sz, pos);

    // telemetry ("!MYYYYXXXX^ hhssaaoooooofffff")
    // ! = message type     hh = heading
    // M = overlay          ss = speed
    // YYYY = latitude      aa = altitude
    // XXXX = longitude     oooooo = orient
    // ^ = symbol           fffff = state flags

    // lat/long
    if (sz < pos + 11)
        return 0; // error too small for lat/long

    this->config.type = data[pos++];
    this->config.overlay = data[pos++];
    base91toBase10(data, pos, decodedNum, 4);
    this->lat = 90.0 - ((double)decodedNum / 380926.0); // reverse magic formula
    base91toBase10(data, pos, decodedNum, 4);
    this->lng = (double)decodedNum / 190463.0 - 180; // reverse magic formula
    this->config.symbol = data[pos++];
    pos++; // skip space

    // heading, speed, alt
    if (sz < pos + 2 + 2 + 3)
        return 0; // error too small for heading, speed, and alt

    base91toBase10(data, pos, decodedNum, 2);
    this->hdg = (double)decodedNum / HDG_SCALE;
    base91toBase10(data, pos, decodedNum, 2);
    this->spd = (double)decodedNum / SPD_SCALE;
    base91toBase10(data, pos, decodedNum, 3);
    this->alt = (double)decodedNum / ALT_SCALE - ALT_OFFSET;

    // orientation
    if (sz < pos + 2 + 2 + 2)
        return 0; // error too small for orientation

    base91toBase10(data, pos, decodedNum, 2);
    this->orient[0] = (double)decodedNum / ORIENTATION_SCALE;
    base91toBase10(data, pos, decodedNum, 2);
    this->orient[1] = (double)decodedNum / ORIENTATION_SCALE;
    base91toBase10(data, pos, decodedNum, 2);
    this->orient[2] = (double)decodedNum / ORIENTATION_SCALE;

    // state info
    if (sz < pos + 5)
        return 0; // error too small for state flags

    base91toBase10(data, pos, decodedNum, 2);
    this->stateFlags += decodedNum << 19;
    base91toBase10(data, pos, decodedNum, 2);
    this->stateFlags += decodedNum << 6;
    base91toBase10(data, pos, decodedNum, 1);
    this->stateFlags += decodedNum;

    return pos;
}