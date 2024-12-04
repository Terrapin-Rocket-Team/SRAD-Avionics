#include "APRSData.h"

uint16_t APRSData::encodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos)
{
    // APRS header ("callsign>tocall,path:")
    if (sz < 29)
        return 0; // error too small for header
    sprintf((char *)data, "%s>%s,%s:", this->config.callsign, this->config.tocall, this->config.path);

    pos += strlen((char *)data);

    return pos;
}

uint16_t APRSData::decodeHeader(uint8_t *data, uint16_t sz, uint16_t &pos)
{
    // APRS header ("callsign>tocall,path:")

    int varPos = 0;
    while (pos < sz && data[pos] != '>')
        this->config.callsign[varPos++] = data[pos++];
    pos++; // skip >
    varPos = 0;
    while (pos < sz && data[pos] != ',')
        this->config.tocall[varPos++] = data[pos++];
    pos++; // skip ,
    varPos = 0;
    while (pos < sz && data[pos] != ':')
        this->config.path[varPos++] = data[pos++];
    pos++; // skip :

    if (pos > sz)
        return 0; // error something went wrong with decoding

    return pos;
}

void APRSData::numtoBase91(uint8_t *str, uint16_t &pos, uint32_t val, int precision)
{
    // precision is the number of digits
    // base 91 conversion process
    for (int i = precision - 1; i >= 0; i--)
    {
        int div = pow(91, i);
        uint8_t num = (uint8_t)((val / div) + 33);
        str[pos++] = num;
        val %= div;
    }
}

void APRSData::base91toNum(uint8_t *str, uint16_t &pos, uint32_t &val, int precision)
{
    // precision is the number of digits
    // reset val to 0
    val = 0;
    // reverse base 91 conversion process
    for (int i = 0; i < precision; i++)
    {
        val += (str[pos++] - 33) * pow(91, precision - i - 1);
    }
}
