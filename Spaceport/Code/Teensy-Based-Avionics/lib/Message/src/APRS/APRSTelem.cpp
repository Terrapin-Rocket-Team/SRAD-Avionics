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

    if (this->config.type != PositionWithoutTimestampWithoutAPRS)
        return 0; // error wrong type, only formatted for PositionWithoutTimestampWithoutAPRS

    data[pos++] = this->config.type;
    data[pos++] = this->config.overlay;
    numtoBase91(data, pos, 380926 * (90 - this->lat), 4);  // magic formula from the APRS spec (page 38)
    numtoBase91(data, pos, 190463 * (180 + this->lng), 4); // magic formula from the APRS spec (page 38)
    data[pos++] = this->config.symbol;
    data[pos++] = ' '; // space to indicate start of comment section

    // heading, speed, alt
    if (sz < pos + 2 + 2 + 3)
        return 0; // error too small for heading, speed, and alt

    numtoBase91(data, pos, (int)(this->hdg * HDG_SCALE), 2);                // (91^2/360) scale to fit in 2 base91 characters
    numtoBase91(data, pos, (int)(this->spd * SPD_SCALE), 2);                // (91^2/1000) scale to fit in 2 base91 characters. 1000 knots is the assumed max speed.
    numtoBase91(data, pos, (int)((this->alt + ALT_OFFSET) * ALT_SCALE), 3); // (91^3/35000) scale to fit in 3 base91 characters. 35000 feet is the assumed max altitude.

    // orientation
    if (sz < pos + 2 + 2 + 2)
        return 0; // error too small for orientation

    numtoBase91(data, pos, (int)(this->orient[0] * ORIENTATION_SCALE), 2); // (91^2/360) scale to fit in 2 base91 characters
    numtoBase91(data, pos, (int)(this->orient[1] * ORIENTATION_SCALE), 2);
    numtoBase91(data, pos, (int)(this->orient[2] * ORIENTATION_SCALE), 2);

    // state info
    if (sz < pos + 5)
        return 0; // error too small for state flags

    // max value is 4294967295, so we need 5 base 91 (or 8 hex) digits to represent it
    numtoBase91(data, pos, this->stateFlags, 5);

    return pos;
}

uint16_t APRSTelem::decode(uint8_t *data, uint16_t sz)
{
    uint16_t pos = 0;
    uint32_t decodedNum = 0;

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
    base91toNum(data, pos, decodedNum, 4);
    this->lat = 90.0 - ((double)decodedNum / 380926.0); // reverse magic formula
    base91toNum(data, pos, decodedNum, 4);
    this->lng = (double)decodedNum / 190463.0 - 180; // reverse magic formula
    this->config.symbol = data[pos++];
    pos++; // skip space

    // heading, speed, alt
    if (sz < pos + 2 + 2 + 3)
        return 0; // error too small for heading, speed, and alt

    base91toNum(data, pos, decodedNum, 2);
    this->hdg = (double)decodedNum / HDG_SCALE;
    base91toNum(data, pos, decodedNum, 2);
    this->spd = (double)decodedNum / SPD_SCALE;
    base91toNum(data, pos, decodedNum, 3);
    this->alt = (double)decodedNum / ALT_SCALE - ALT_OFFSET;

    // orientation
    if (sz < pos + 2 + 2 + 2)
        return 0; // error too small for orientation

    base91toNum(data, pos, decodedNum, 2);
    this->orient[0] = (double)decodedNum / ORIENTATION_SCALE;
    base91toNum(data, pos, decodedNum, 2);
    this->orient[1] = (double)decodedNum / ORIENTATION_SCALE;
    base91toNum(data, pos, decodedNum, 2);
    this->orient[2] = (double)decodedNum / ORIENTATION_SCALE;

    // state info
    if (sz < pos + 5)
        return 0; // error too small for state flags

    base91toNum(data, pos, decodedNum, 5);
    this->stateFlags += decodedNum;

    return pos;
}

uint16_t APRSTelem::toJSON(char *json, uint16_t sz, const char *streamName)
{
    uint16_t result = (uint16_t)snprintf(json, sz, "{\"type\": \"APRSTelem\", \"name\":\"%s\", \"data\": {\"lat\": %.7lf, \"lng\": %.7lf, \"alt\": %lf, \"spd\": %lf, \"hdg\": %lf, \"orient\": [%lf, %lf, %lf], \"stateflags\": \"%#lx\"}}", streamName, this->lat, this->lng, this->alt, this->spd, this->hdg, this->orient[0], this->orient[1], this->orient[2], this->stateFlags);

    if (result < sz)
    {
        // ran properly
        return result;
    }
    // output too large
    return 0;
}

uint16_t APRSTelem::fromJSON(char *json, uint16_t sz, char *streamName)
{
    char lat[14] = {0};
    char lng[14] = {0};
    char alt[14] = {0};
    char spd[14] = {0};
    char hdg[14] = {0};
    char sf[11] = {0};
    if (!extractStr(json, sz, "\"name\":\"", '"', streamName))
        return 0;
    if (!extractStr(json, sz, "\"lat\": ", ',', lat, 14))
        return 0;
    if (!extractStr(json, sz, "\"lng\": ", ',', lng, 14))
        return 0;
    if (!extractStr(json, sz, "\"alt\": ", ',', alt, 14))
        return 0;
    if (!extractStr(json, sz, "\"spd\": ", ',', spd, 14))
        return 0;
    if (!extractStr(json, sz, "\"hdg\": ", ',', hdg, 14))
        return 0;
    if (!extractStr(json, sz, "\"stateflags\": \"", '"', sf, 11))
        return 0;

    this->lat = atof(lat);
    this->lng = atof(lng);
    this->alt = atof(alt);
    this->spd = atof(spd);
    this->hdg = atof(hdg);
    this->stateFlags = strtol(sf, NULL, 16);

    char *orientStrPos = strstr(json, "\"orient\": [");
    int orientPos = int(orientStrPos - json) + 11;
    int orientIndex = 0;
    char orientStr[14] = {0};
    while (json[orientPos] != ']' && orientPos < sz && orientIndex < 3)
    {
        int counter = 0;
        while (json[orientPos] != ',' && json[orientPos] != ']' && orientPos < sz)
        {
            orientStr[counter++] = json[orientPos++];
        }
        this->orient[orientIndex++] = atof(orientStr);
        orientPos++;
    }

    return 1;
}