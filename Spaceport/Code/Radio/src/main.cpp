#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_40k,    // data rate
    433e6,     // frequency (Hz)
    127,       // tx power (127 = ~20dBm)
    48,        // preamble length
    16,        // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();
uint32_t timeout = 2100;

uint32_t received = 0;
uint32_t timeouts = 0;

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};

APRSTelem testMessage(aprscfg);

// void logStats();

char *s_min_nn(uint32_t min_nnnnn, int high_precision);
String create_lat_aprs(String lat, bool hp);
String create_long_aprs(String lng, bool hp);
String create_dao_aprs(String lat, String lng);
String padding(unsigned int number, unsigned int width);

void setup()
{
    Serial.begin(115200);
    if (!radio.begin())
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
            ;
    }
    Serial.println("Radio began successfully");
}

int spd_zero_counter = 0;

void loop()
{
    if (radio.avail())
    {
        radio.receive(testMessage);
        // Serial.println((char *)radio.buf);
        Serial.print("s\r\n");
        Serial.print("Source:");
        Serial.print(testMessage.config.callsign);
        Serial.print(",Destination:");
        Serial.print(testMessage.config.tocall);
        Serial.print(",Path:");
        Serial.print("WIDE1-1");
        Serial.print(",Type:");
        Serial.print(testMessage.config.type);
        Serial.print(",Data:");

        // Quarantine the old code

        String body;
        // format lat/long
        String dao = create_dao_aprs(String(testMessage.lat), String(testMessage.lng));
        String lat = create_lat_aprs(String(testMessage.lat), 1);
        String lng = create_long_aprs(String(testMessage.lng), 1);

        // add alt
        int alt_int = max(-99999, min(999999, testMessage.alt));
        String alt;
        if (alt_int < 0)
        {
            alt = "/A=-" + padding(alt_int * -1, 5);
        }
        else
        {
            alt = "/A=" + padding(alt_int, 6);
        }

        // add course/speed
        int spd_int = max(0, min(999, testMessage.spd));
        String hdg_spd = "";
        String spd;
        String hdg;
        if (spd_int == 0 && spd_zero_counter < 3)
        {
            spd_zero_counter++;
        }
        else if (spd_int != 0)
        {
            spd_zero_counter = 0;
        }
        // we don't want to keep sending that our speed is 0, so we stop after the second time
        if (spd_zero_counter < 3)
        {
            int hdg_int = max(0, min(360, testMessage.hdg));
            if (hdg_int == 0)
                hdg_int = 360;
            spd = padding(spd_int, 3);
            hdg = padding(hdg_int, 3);
            hdg_spd = hdg + "/" + spd;
        }

        // put it together
        body = "!" + lat + "/" + lng + "[" + hdg_spd + alt;

        // add any other info
        body += "/S" + String(testMessage.stateFlags & 0x0000000f) + "/00:00:00";

        // add dao
        body += " " + dao;

        // Quarantine the old code
        Serial.print(body);
        Serial.print(",RSSI:");
        Serial.print(radio.RSSI());
        Serial.print("\r\ne\r\n");

        // reset timeout
        timer = millis();
        received++;
        // logStats();
    }
    if (millis() - timer > timeout)
    {
        timer = millis();
        timeouts++;
        // logStats();
    }
    // need to call as fast as possible every loop
    radio.update();
}

// void logStats()
// {
//     Serial.print("Received: ");
//     Serial.print(received);
//     Serial.print(" | Timeouts: ");
//     Serial.println(timeouts);
// }

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