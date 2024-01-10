/*
MIT License

Copyright (c) 2020 Peter Buchegger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef APRS_ENCODE_FUNCTIONS_H
#define APRS_ENCODE_FUNCTIONS_H
#include <Arduino.h>

char *s_min_nn(uint32_t min_nnnnn, int high_precision);
String create_lat_aprs(String lat, bool hp);
String create_long_aprs(String lng, bool hp);
String create_dao_aprs(String lat, String lng);
String padding(unsigned int number, unsigned int width);

// takes in decimal minutes and converts to MM.dd or MM.dddd
char *s_min_nn(uint32_t min_nnnnn, int high_precision)
{
    /* min_nnnnn: RawDegrees billionths is uint32_t by definition and is n'telth
     * degree (-> *= 6 -> nn.mmmmmm minutes) high_precision: 0: round at decimal
     * position 2. 1: round at decimal position 4. 2: return decimal position 3-4
     * as base91 encoded char
     */

    static char buf[6];
    min_nnnnn = min_nnnnn * 0.006;

    if (high_precision)
    {
        if ((min_nnnnn % 10) >= 5 && min_nnnnn < 6000000 - 5)
        {
            // round up. Avoid overflow (59.999999 should never become 60.0 or more)
            min_nnnnn = min_nnnnn + 5;
        }
    }
    else
    {
        if ((min_nnnnn % 1000) >= 500 && min_nnnnn < (6000000 - 500))
        {
            // round up. Avoid overflow (59.9999 should never become 60.0 or more)
            min_nnnnn = min_nnnnn + 500;
        }
    }

    if (high_precision < 2)
        sprintf(buf, "%02u.%02u", (unsigned int)((min_nnnnn / 100000) % 100), (unsigned int)((min_nnnnn / 1000) % 100));
    else
        sprintf(buf, "%c", (char)((min_nnnnn % 1000) / 11) + 33);
    // Like to verify? type in python for i.e. RawDegrees billions 566688333: i =
    // 566688333; "%c" % (int(((i*.0006+0.5) % 100)/1.1) +33)
    return buf;
}

// creates the latitude string for the APRS message based on whether the GPS coordinates are high precision
String create_lat_aprs(String lat, bool hp)
{
    char str[20];
    char n_s = 'N';
    if (lat.charAt(0) == '-')
    {
        n_s = 'S';
        lat = lat.substring(1);
    }
    // we like sprintf's float up-rounding.
    // but sprintf % may round to 60.00 -> 5360.00 (53° 60min is a wrong notation
    // ;)
    sprintf(str, "%02d%s%c", lat.substring(0, lat.indexOf('.')).toInt(), s_min_nn(lat.substring(lat.indexOf('.') + 1).toInt(), hp), n_s);
    String lat_str(str);
    return lat_str;
}

// creates the longitude string for the APRS message based on whether the GPS coordinates are high precision
String create_long_aprs(String lng, bool hp)
{
    char str[20];
    char e_w = 'E';
    if (lng.charAt(0) == '-')
    {
        e_w = 'W';
        lng = lng.substring(1);
    }
    sprintf(str, "%03d%s%c", lng.substring(0, lng.indexOf('.')).toInt(), s_min_nn(lng.substring(lng.indexOf('.') + 1).toInt(), hp), e_w);
    String lng_str(str);
    return lng_str;
}

// creates the dao at the end of aprs message based on latitude and longitude
String create_dao_aprs(String lat, String lng)
{
    // !DAO! extension, use Base91 format for best precision
    // /1.1 : scale from 0-99 to 0-90 for base91, int(... + 0.5): round to nearest
    // integer https://metacpan.org/dist/Ham-APRS-FAP/source/FAP.pm
    // http://www.aprs.org/aprs12/datum.txt
    //

    char str[10];
    // show_display("Test", lat.substring(lat.indexOf('.') + 1).toInt(), lng.substring(lng.indexOf('.') + 1).toInt());
    sprintf(str, "!w%s", s_min_nn(lat.substring(lat.indexOf('.') + 1).toInt(), 2));
    sprintf(str + 3, "%s!", s_min_nn(lng.substring(lng.indexOf('.') + 1).toInt(), 2));
    String dao_str(str);
    return dao_str;
}

// adds a specified number of zeros to the begining of a number
String padding(unsigned int number, unsigned int width)
{
    String result;
    String num(number);
    if (num.length() > width)
    {
        width = num.length();
    }
    for (unsigned int i = 0; i < width - num.length(); i++)
    {
        result.concat('0');
    }
    result.concat(num);
    return result;
}

#endif