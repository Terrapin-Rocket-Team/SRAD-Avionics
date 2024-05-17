#include "APRSTelemMsg.h"

APRSTelemMsg::APRSTelemMsg(APRSHeader header) : APRSMsgBase(header)
{
    data.lat = 0;
    data.lng = 0;
    data.alt = 0;
    data.spd = 0;
    data.hdg = 0;
    data.stage = 0;
    data.orientation = imu::Vector<3>(0, 0, 0);
    data.statusFlags = 0;
}

void APRSTelemMsg::encodeData(int cursor)
{
    /* Small Aircraft symbol ( /' ) probably better than Jogger symbol ( /[ ) lol.
     I'm going to use the Overlayed Aircraft symbol ( \^ ) for now, with the overlay of 'M' for UMD ( M^ ).
     Uses base91 encoding to compress message as much as possible. APRS messages are apparently supposed to be only 256 bytes.

     Takes the regular format of !DDMM.hhN/DDDMM.hhW^ALTSPD/HDG SSOrientationF

     which expands to !(lat)/(lng)^altspd/hdg (stage #)(orientation)(status flags)

     and compresses it to /YYYYXXXX^ ccssaasoooooof

     which expands to /(lat)(lng)^ (course)(speed)(altitude)(stage)(orientation [as euler angles])(status flags)

     Specifically does not use the csT compressed format because we don't need T and want better accuracy on course angle.
     for more information on APRS, see http://www.aprs.org/doc/APRS101.PDF


     lat and lng are in decimal degrees. no need to convert to DMS.
     course is in degrees, speed is in knots, altitude is in feet, stage is an integer, orientation is in degrees, Pi status is a byte of flags.
    */

    // lat and lng
    string[cursor++] = '!';                                          // Message type (position without timestamp)
    string[cursor++] = 'M';                                          // overlay
    encodeBase91(string, cursor, (int)380926 * (90 - data.lat), 4);  // 380926 is the magic number from the APRS spec (page 38)
    encodeBase91(string, cursor, (int)190463 * (180 + data.lng), 4); // 190463 is the magic number from the APRS spec (page 38)
    string[cursor++] = '^';                                          // end of lat and lng
    string[cursor++] = ' ';                                          // space to start 'Comment' section

    // course, speed, altitude
    encodeBase91(string, cursor, (int)(data.hdg * HDG_SCALE), 2);                // (91^2/360) scale to fit in 2 base91 characters
    encodeBase91(string, cursor, (int)(data.spd * SPD_SCALE), 2);                // (91^2/1000) scale to fit in 2 base91 characters. 1000 knots is the assumed max speed.
    encodeBase91(string, cursor, (int)((data.alt + ALT_OFFSET) * ALT_SCALE), 2); // (91^2/15000) scale to fit in 2 base91 characters. 15000 feet is the assumed max altitude.

    // stage and orientation
    string[cursor++] = (uint8_t)(data.stage + (int)'0');                              // stage is just written in plaintext.
    encodeBase91(string, cursor, (int)(data.orientation.x() * ORIENTATION_SCALE), 2); // same as course
    encodeBase91(string, cursor, (int)(data.orientation.y() * ORIENTATION_SCALE), 2); // same as course
    encodeBase91(string, cursor, (int)(data.orientation.z() * ORIENTATION_SCALE), 2); // same as course
    string[cursor++] = (uint8_t)(data.statusFlags + 33);                              // all content is supposed to be printable ASCII characters, so we add 33 to the status flags to make sure it is.
    len = cursor;
}

void APRSTelemMsg::decodeData(int cursor)
{
    // format is /YYYYXXXX^ ccssaasoooooof

    // lat and lng
    cursor++; // skip '!'
    cursor++; // skip overlay
    decodeBase91(string, cursor, data.lat, 4);
    data.lat = 90 - data.lat / 380926.0;
    decodeBase91(string, cursor, data.lng, 4);
    data.lng = data.lng / 190463.0 - 180;
    cursor++; // skip '^'
    cursor++; // skip ' '

    // course, speed, altitude
    decodeBase91(string, cursor, data.hdg, 2);
    data.hdg /= HDG_SCALE;
    decodeBase91(string, cursor, data.spd, 2);
    data.spd /= SPD_SCALE;
    decodeBase91(string, cursor, data.alt, 2);
    data.alt = data.alt / ALT_SCALE - ALT_OFFSET;

    // stage and orientation
    data.stage = string[cursor++] - (int)'0';
    decodeBase91(string, cursor, data.orientation.x(), 2);
    data.orientation.x() /= ORIENTATION_SCALE;
    decodeBase91(string, cursor, data.orientation.y(), 2);
    data.orientation.y() /= ORIENTATION_SCALE;
    decodeBase91(string, cursor, data.orientation.z(), 2);
    data.orientation.z() /= ORIENTATION_SCALE;
    data.statusFlags = string[cursor++] - 33;
}
