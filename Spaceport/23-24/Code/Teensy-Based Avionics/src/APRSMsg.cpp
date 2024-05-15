#include "APRSMsg.h"


#define MAX_ALLOWED_MSG_LEN 255 /* max length of 1 message supported by radio buffer */



APRSMsg::APRSMsg(APRSHeader &header)
{
    this->header = header;
}

const uint8_t *APRSMsg::encode()
{
    char *msg = new char[MAX_ALLOWED_MSG_LEN];
    int c = encodeHeader(msg);
    encodeData(msg, c);
    return (uint8_t *)msg;
}

bool APRSMsg::decode(const uint8_t *message, int len)
{
    //message needs to be decoded and values reinstered into the header and data structs
    int c = decodeHeader((char *)message, len);
    decodeData((char *)message, len, c);
    return false;
}

#pragma region Encode Helpers

int APRSMsg::encodeHeader(char *message) const
{
    // format: CALLSIGN>TOCALL,PATH:
    int cursor = 0;
    for (int i = 0; header.CALLSIGN[i] && i < 8; i++, cursor++)
    {
        message[cursor] = header.CALLSIGN[i];
    }
    message[cursor++] = '>';
    for (int i = 0; header.TOCALL[i] && i < 8; i++, cursor++)
    {
        message[cursor] = header.TOCALL[i];
    }
    message[cursor++] = ',';
    for (int i = 0; header.PATH[i] && i < 10; i++, cursor++)
    {
        message[cursor] = header.PATH[i];
    }
    message[cursor++] = ':'; // end of header
    return cursor;
}

void APRSMsg::encodeData(char *message, int cursor)
{
    /* Small Aircraft symbol ( /' ) probably better than Jogger symbol ( /[ ) lol.
     I'm going to use the Overlayed Aircraft symbol ( \^ ) for now, with the overlay of 'M' for UMD ( M^ ).
     Uses base91 encoding to compress message as much as possible. APRS messages are apparently supposed to be only 256 bytes.


     takes the regular format of !DDMM.hhN/DDDMM.hhW^ALTSPD/HDG SSOrientation

     which expands to !(lat)/(lng)^altspd/hdg (stage #)(orientation)

     and compresses it to /YYYYXXXX^ ccssaasoooooo

     which expands to /(lat)(lng)^ (course)(speed)(altitude)(stage)(orientation [as euler angles])

     specifically does not use the csT compressed format because we don't need T and want better accuracy on course angle.
     for more information on APRS, see http://www.aprs.org/doc/APRS101.PDF


     lat and lng are in decimal degrees. no need to convert to DMS.
     course is in degrees, speed is in knots, altitude is in feet, stage is an integer, orientation is in degrees.
    */

    // lat and lng
    message[cursor++] = 'M';                                            // overlay
    encodeBase91(message, cursor, (int)380926 * (90 - data.lat), 4);  // 380926 is the magic number from the APRS spec (page 38)
    encodeBase91(message, cursor, (int)190463 * (180 + data.lng), 4); // 190463 is the magic number from the APRS spec (page 38)
    message[cursor++] = '^';                                            // end of lat and lng
    message[cursor++] = ' ';                                            // space to start 'Comment' section

    // course, speed, altitude
    encodeBase91(message, cursor, (int)(data.hdg * HDG_SCALE), 2);   // (91^2/360) scale to fit in 2 base91 characters
    encodeBase91(message, cursor, (int)(data.spd * SPD_SCALE), 2);  // (91^2/1000) scale to fit in 2 base91 characters. 1000 knots is the assumed max speed.
    encodeBase91(message, cursor, (int)(data.alt * ALT_SCALE), 2); // (91^2/15000) scale to fit in 2 base91 characters. 15000 feet is the assumed max altitude.

    // stage and orientation
    message[cursor++] = (char)(data.stage + (int)'0');                              // stage is just written in plaintext.
    encodeBase91(message, cursor, (int)(data.orientation.x() * ORIENTATION_SCALE), 2); // same as course
    encodeBase91(message, cursor, (int)(data.orientation.y() * ORIENTATION_SCALE), 2); // same as course
    encodeBase91(message, cursor, (int)(data.orientation.z() * ORIENTATION_SCALE), 2); // same as course

    len = cursor;
}

#pragma endregion

#pragma region Decode Helpers

int APRSMsg::decodeHeader(const char *message, int len)
{
    // format: CALLSIGN>TOCALL,PATH:
    int cursor = 0;
    for (int i = 0; message[cursor] != '>'; i++, cursor++)
    {
        header.CALLSIGN[i] = message[cursor];
    }
    header.CALLSIGN[cursor] = '\0';
    cursor++; // skip '>'
    for (int i = 0; message[cursor] != ','; i++, cursor++)
    {
        header.TOCALL[i] = message[cursor];
    }
    header.TOCALL[cursor] = '\0';
    cursor++; // skip ','
    for (int i = 0; message[cursor] != ':'; i++, cursor++)
    {
        header.PATH[i] = message[cursor];
    }
    header.PATH[cursor] = '\0';
    cursor++; // skip ':'
    return cursor;
}

void APRSMsg::decodeData(const char *message, int len, int cursor)
{
    // lat and lng
    cursor++; // skip overlay
    decodeBase91(message, cursor, data.lat, 4);
    data.lat = 90 - data.lat / 380926.0;
    decodeBase91(message, cursor, data.lng, 4);
    data.lng = data.lng / 190463.0 - 180;
    cursor++; // skip '^'
    cursor++; // skip ' '

    // course, speed, altitude
    decodeBase91(message, cursor, data.hdg, 2);
    data.hdg /= HDG_SCALE;
    decodeBase91(message, cursor, data.spd, 2);
    data.spd /= SPD_SCALE;
    decodeBase91(message, cursor, data.alt, 2);
    data.alt /= ALT_SCALE;

    // stage and orientation
    data.stage = message[cursor++] - (int)'0';
    decodeBase91(message, cursor, data.orientation.x(), 2);
    data.orientation.x() /= ORIENTATION_SCALE;
    decodeBase91(message, cursor, data.orientation.y(), 2);
    data.orientation.y() /= ORIENTATION_SCALE;
    decodeBase91(message, cursor, data.orientation.z(), 2);
    data.orientation.z() /= ORIENTATION_SCALE;
}

#pragma endregion

#pragma region Base91 Encoding

void APRSMsg::encodeBase91(char *message, int &cursor, int value, int precision) const
{
    for (int i = precision - 1; i >= 0; i--)
    {
        int divisor = pow(91, i);
        message[cursor++] = (char)((int)(value / divisor) + 33);
        value %= divisor;
    }
}

void APRSMsg::decodeBase91(const char *message, int &cursor, double &value, int precision) const
{
    value = 0;
    for (int i = 0; i < precision; i++)
    {
        value += (message[cursor++] - 33) * pow(91, precision - i - 1);
    }
}

#pragma endregion