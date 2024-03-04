#include "RFM69HCW.h"

/*
Constructor
    - frequency in hertz
    - transmitter true if the radio will be on the rocket
    - highBitrate true for 300kbps, false for 4.8kbps
    - config with APRS settings
*/
RFM69HCW::RFM69HCW(const RadioSettings *s, const APRSConfig *config) : radio(s->cs, s->irq, *s->spi)
{

    this->settings = *s;
    this->cfg = *config;
    this->avail = false;
    this->msgLen = -1;

    if (this->settings.transmitter)
    {
        // addresses for transmitters
        this->addr = 0x02;
        this->toAddr = 0x01;
    }
    else
    {
        // addresses for receivers
        this->addr = 0x01;
        this->toAddr = 0x02;
    }
    this->id = 0x00; // to fit the max number of bytes in a packet, this will change
                     // based on message length, but it will be reset to this value
                     // after each complete transmission/reception
}

/*
Initializer to call in setup
*/
bool RFM69HCW::begin()
{
    // reset the radio
    pinMode(this->settings.rst, OUTPUT);
    digitalWrite(this->settings.rst, LOW);
    delay(10);
    digitalWrite(this->settings.rst, HIGH);
    delay(10);
    digitalWrite(this->settings.rst, LOW);

    if (!this->radio.init())
        return false;

    // then use this to actually set the frequency
    if (!this->radio.setFrequency(this->settings.frequency))
        return false;

    // set transmit power
    this->radio.setTxPower(this->txPower, true);

    // always set FSK mode
    this->radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);

    // set headers
    this->radio.setHeaderTo(this->toAddr);
    this->radio.setHeaderFrom(this->addr);
    this->radio.setThisAddress(this->addr);
    this->radio.setHeaderId(this->id);

    if (this->settings.highBitrate) // Note: not working
    {
        // the default bitrate of 4.8kbps should be fine unless we want high bitrate for video
        // set300KBPS();
        this->radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);
        // remove as much overhead as possible
        // this->radio.setPreambleLength(0);
        // this->radio.setSyncWords();
    }

    return true;
}

/*
Most basic transmission method, simply transmits the string without modification
    \param message is the message to be transmitted, set to nullptr if this->msg already contains the message
    \param len [optional] is the length of the message, if not used it is assumed this->msgLen is already set to the length
*/
bool RFM69HCW::tx(const char *message, int len)
{
    // figure out how long the message should be
    if (len > 0)
        this->msgLen = len > MSG_LEN ? MSG_LEN : len;
    else if (this->msgLen == -1)
        return false;

    // reset variables for tx
    this->id = 0x00;

    // get number of packets for this message, and set this radio's id to that value so the receiver knows how many packets to expect
    this->totalPackets = len / this->bufSize + (len % this->bufSize > 0);
    this->radio.setHeaderId(this->id);

    if (message != nullptr)
        memcpy(this->msg, message, len);

    return true;
}

bool RFM69HCW::sendBuffer()
{
    memcpy(this->buf, this->msg + this->id * this->bufSize, min(this->bufSize, this->msgLen - this->id * this->bufSize));
    this->radio.send(this->buf, min(this->bufSize, this->msgLen - this->id * this->bufSize));
    this->id++;
    return this->id == this->totalPackets;
}

void RFM69HCW::endtx()
{
    this->msgLen = -1;
    this->id = 0x00;
    this->radio.setHeaderId(this->id);
}

/*
Receive function
Most basic receiving method, simply checks for a message and returns a pointer to it
Note that the message may be incomplete, if the message is complete available() will return true
The received message must be less than MSG_LEN
*/
const char *RFM69HCW::rx()
{
    if (this->radio.available())
    {
        // Should be a message for us now
        uint8_t receivedLen = this->bufSize;
        if (this->radio.recv(this->buf, &(receivedLen)))
        {
            // check if this is the first part of the message
            if (this->totalPackets == 0)
            {
                this->totalPackets = this->radio.headerId();
                if (this->totalPackets <= 0)
                    return 0;
                this->msgLen = 0;
                this->rssi = 0;
            }

            // copy data from this->buf to this->msg, making sure not to overflow
            memcpy(this->msg + this->msgLen, this->buf, min(MSG_LEN - (this->msgLen + receivedLen), receivedLen));

            this->msgLen += receivedLen;
            this->id++;
            this->rssi += radio.lastRssi();

            // check if the full message has been received
            if (this->totalPackets == this->id)
            {
                this->id = 0x00;
                this->rssi /= this->totalPackets;
                this->totalPackets = 0;
                this->avail = true;
            }

            return this->msg;
        }
        return 0;
    }
    return 0;
}

/*
\return ```true``` if the radio is currently busy
*/
bool RFM69HCW::busy()
{
    RHGenericDriver::RHMode mode = this->radio.mode();
    if (mode == RHGenericDriver::RHModeTx)
        return true;
    return false;
}

/*
Multi-purpose encoder function
Encodes the message into a format selected by type
char *message is the message to be encoded
sets this->msgLen to the encoded length of message
- Telemetry:
    message - input: latitude,longitude,altitude,speed,heading,precision,stage,t0 -> output: APRS message
- Video:
    message - input: char* filled with raw bytes -> output: char* filled with raw bytes + error checking
- Ground Station: TODO
    message - input: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value -> output: APRS message
*/
bool RFM69HCW::encode(char *message, EncodingType type, int len)
{
    if (type == ENCT_TELEMETRY)
    {
        if (len > 0)
            this->msgLen = len > MSG_LEN ? MSG_LEN : len;
        else if (this->msgLen == -1)
            return false;

        // holds the data to be assembled into the aprs body
        APRSData data;

        // find each value separated in order by a comma and put in the APRSData array
        {
            char *currentVal = new char[this->msgLen];
            int currentValIndex = 0;
            int currentValCount = 0;
            for (int i = 0; i < msgLen; i++)
            {
                if (message[i] != ',')
                {
                    currentVal[currentValIndex] = message[i];
                    currentValIndex++;
                }
                if (message[i] == ',' || (currentValCount == 7 && i == msgLen - 1))
                {
                    currentVal[currentValIndex] = '\0';

                    if (currentValCount == 0 && strlen(currentVal) < 16)
                        strcpy(data.lat, currentVal);
                    if (currentValCount == 1 && strlen(currentVal) < 16)
                        strcpy(data.lng, currentVal);
                    if (currentValCount == 2 && strlen(currentVal) < 10)
                        strcpy(data.alt, currentVal);
                    if (currentValCount == 3 && strlen(currentVal) < 4)
                        strcpy(data.spd, currentVal);
                    if (currentValCount == 4 && strlen(currentVal) < 4)
                        strcpy(data.hdg, currentVal);
                    if (currentValCount == 5)
                        data.precision = currentVal[0];
                    if (currentValCount == 6 && strlen(currentVal) < 3)
                        strcpy(data.stage, currentVal);
                    if (currentValCount == 7 && strlen(currentVal) < 9)
                        strcpy(data.t0, currentVal);

                    currentValIndex = 0;
                    currentValCount++;
                }
            }

            delete[] currentVal;
        }

        // get lat and long string for low or high precision
        if (data.precision == 'L')
        {
            strcpy(data.dao, "");
            create_lat_aprs(data.lat, 0);
            create_long_aprs(data.lng, 0);
        }
        else if (data.precision == 'H')
        {
            create_dao_aprs(data.lat, data.lng, data.dao);
            create_lat_aprs(data.lat, 1);
            create_long_aprs(data.lng, 1);
        }

        // get alt string
        int alt_int = max(-99999, min(999999, atoi(data.alt)));
        if (alt_int < 0)
        {
            strcpy(data.alt, "/A=-");
            padding(alt_int * -1, 5, data.alt, 4);
        }
        else
        {
            strcpy(data.alt, "/A=");
            padding(alt_int, 6, data.alt, 3);
        }

        // get course/speed strings
        // TODO add speed zero counter (makes decoding more complex)
        int spd_int = max(0, min(999, atoi(data.spd)));
        int hdg_int = max(0, min(360, atoi(data.hdg)));
        if (hdg_int == 0)
            hdg_int = 360;
        padding(spd_int, 3, data.spd);
        padding(hdg_int, 3, data.hdg);

        // generate the aprs message
        APRSMsg aprs;

        aprs.setSource(this->cfg.CALLSIGN);
        aprs.setPath(this->cfg.PATH);
        aprs.setDestination(this->cfg.TOCALL);

        char body[80];
        sprintf(body, "%c%s%c%s%c%s%c%s%s%s%s%c%s%c%s", '!', data.lat, this->cfg.OVERLAY, data.lng, this->cfg.SYMBOL,
                data.hdg, '/', data.spd, data.alt, "/S", data.stage, '/',
                data.t0, ' ', data.dao);
        aprs.getBody()->setData(body);
        aprs.encode(message);
        return true;
    }
    if (type == ENCT_VIDEO)
    {
        // lower bound for received message length, for basic error checking
        if (len > 0)
            this->msgLen = len > MSG_LEN ? MSG_LEN : len;
        // if len wasn't set, make sure this->msgLen is
        if (this->msgLen == -1)
            return false;
        message[this->msgLen] = this->msgLen / 255;
    }
    if (type == ENCT_NONE)
        return true;
    return false;
}

/*
Multi-purpose decoder function
Decodes the message into a format selected by type
char *message is the message to be decoded
sets this->msgLen to the length of the decoded message
- Telemetry: TODO: fix
    message - output: latitude,longitude,altitude,speed,heading,precision,stage,t0 <- input: APRS message
- Video: TODO
    message - output: char* filled with raw bytes <- input: Raw byte array
- Ground Station:
    message - output: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value <- input: APRS message
*/
bool RFM69HCW::decode(char *message, EncodingType type, int len)
{
    if (type == ENCT_TELEMETRY)
    {
        if (len > 0)
            this->msgLen = len > MSG_LEN ? MSG_LEN : len;
        else if (this->msgLen == -1)
            return false;

        // make sure message is null terminated
        message[this->msgLen] = 0;

        APRSMsg aprs;
        aprs.decode(message);

        char body[80];
        char *bodyptr = body;
        strcpy(body, aprs.getBody()->getData());
        // decode body
        APRSData data;
        int i = 0;
        int len = strlen(body);

        // TODO this could probably be shortened
        // body should start with '!'
        if (body[0] != '!')
            return false;
        i++;
        bodyptr = body + i;

        // find latitude
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.lat, bodyptr, i - (bodyptr - body));
        data.lat[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find longitude
        while (body[i] != '[' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.lng, bodyptr, i - (bodyptr - body));
        data.lng[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find heading
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.hdg, bodyptr, i - (bodyptr - body));
        data.hdg[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find speed
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.spd, bodyptr, i - (bodyptr - body));
        data.spd[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find altitude
        if (body[i] != 'A' && body[i + 1] != '=')
            return false;
        i += 2;
        bodyptr = body + i;
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.alt, bodyptr, i - (bodyptr - body));
        data.alt[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find stage
        if (body[i] != 'S')
            return false;
        i++;
        bodyptr = body + i;
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.stage, bodyptr, i - (bodyptr - body));
        data.stage[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find t0
        while (body[i] != ' ' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.t0, bodyptr, i - (bodyptr - body));
        data.t0[i - (bodyptr - body)] = 0;
        i++;
        bodyptr = body + i;

        // find dao
        strncpy(data.dao, bodyptr, len - (bodyptr - body));
        data.dao[len - (bodyptr - body)] = '\0';

        // convert lat and lng to degrees

        int latMult = (data.lat[strlen(data.lat) - 1] == 'N') ? 1 : -1;
        int lngMult = (data.lat[strlen(data.lat) - 1] == 'E') ? 1 : -1;

        int lenLat = strlen(data.lat);
        int decimalPosLat = 0;
        // use for loop in case there is no decimal
        for (int i = 0; i < lenLat; i++)
        {
            if (data.lat[i] == '.')
            {
                decimalPosLat = i;
                break;
            }
        }

        int lenLng = strlen(data.lng);
        int decimalPosLng = 0;
        // use for loop in case there is no decimal
        for (int i = 0; i < lenLng; i++)
        {
            if (data.lng[i] == '.')
            {
                decimalPosLng = i;
                break;
            }
        }

        double lat = 0;
        for (int i = decimalPosLat - 3; i >= 0; i--)
        {
            int t = data.lat[i];
            for (int j = 0; j < i; j++)
                t *= 10;
            lat += t;
        }

        double latMins = 0;
        for (int i = lenLat - 2; i > decimalPosLat - 3; i--)
        {
            if (data.lat[i] == '.')
                continue;
            double t = data.lat[i];
            for (int j = (lenLat - 2 - decimalPosLat) * -1; j < i - 2; j++)
                t *= j < 0 ? 1 / 10 : 10;
            latMins += t;
        }
        latMins /= 60;

        lat += latMins;

        double lng = 0;
        for (int i = decimalPosLng - 3; i >= 0; i--)
        {
            int t = data.lng[i];
            for (int j = 0; j < i; j++)
                t *= 10;
            lat += t;
        }

        double lngMins = 0;
        for (int i = lenLng - 2; i > decimalPosLng - 3; i--)
        {
            if (data.lng[i] == '.')
                continue;
            double t = data.lng[i];
            for (int j = (lenLng - 2 - decimalPosLng) * -1; j < i - 2; j++)
                t *= j < 0 ? 1 / 10 : 10;
            lngMins += t;
        }
        lngMins /= 60;

        lng += lngMins;

        lat *= latMult;
        lng *= lngMult;

        sprintf(message, "%f,%f,%s,%s,%s,%c,%s,%s", lat, lng, data.alt, data.spd, data.hdg, strlen(data.dao) > 0 ? 'H' : 'L', data.stage, data.t0);

        this->msgLen = strlen(message);
        return true;
    }
    if (type == ENCT_GROUNDSTATION)
    {
        if (len > 0)
            this->msgLen = len > MSG_LEN ? MSG_LEN : len;
        else if (this->msgLen == -1)
            return false;

        // make sure message is null terminated
        message[this->msgLen] = 0;

        // put the message into a APRSMessage object to decode it
        APRSMsg aprs;
        aprs.decode(message);
        aprs.toString(message);

        // add RSSI to the end of message
        sprintf(message + strlen(message), "%s%d", ",RSSI:", this->rssi);

        this->msgLen = strlen(message);
        return true;
    }
    if (type == ENCT_VIDEO)
    {
        if (len > 0)
            this->msgLen = len > MSG_LEN ? MSG_LEN : len;

        // if len wasn't set, make sure this->msgLen is
        if (this->msgLen == -1)
            return false;

        // otherwise, check if the length of the message is within the expected bounds
        return this->msgLen >= message[this->msgLen] * 255 && this->msgLen < (message[this->msgLen] + 1) * 255;
    }
    if (type == ENCT_NONE)
        return true;
    return false;
}

/*
Encodes the message into the selected type, then sends it. Transmitted and encoded message length must not exceed ```MSG_LEN```
    \param message is the message to be sent
    \param type is the encoding type
    \param len optional length of message, required if message is not a null terminated string
\return ```true``` if the message was sent successfully
*/
bool RFM69HCW::send(const char *message, EncodingType type, int len)
{
    if (type == ENCT_TELEMETRY || type == ENCT_GROUNDSTATION)
    {
        // assume message is a valid string, but maybe it's not null terminated
        strncpy(this->msg, message, MSG_LEN);
        this->msg[MSG_LEN] = 0; // so null terminate it just in case
        // figure out what the length of the message should be
        if (len > 0 && len <= MSG_LEN)
            this->msgLen = len;
        else
            this->msgLen = strlen(this->msg);
        return encode(this->msg, type, this->msgLen) && tx(nullptr, strlen(this->msg)); // length needs to be updated since encode() modifies the message
    }
    if (type == ENCT_VIDEO && len != -1)
    {
        // can't assume message is a valid string
        this->msgLen = len > MSG_LEN ? MSG_LEN : len;
        memcpy(this->msg, message, this->msgLen);
        return encode(this->msg, type) && tx(nullptr); // length does not change with ENCT_VIDEO
    }
    if (type == ENCT_NONE && len != -1)
    {
        return tx(message, len);
    }
    return false;
}

/*
Comprehensive receive function
Should be called after verifying there is an available message by calling available()
Decodes the last received message according to the type
Received and decoded message length must not exceed MSG_LEN
    \param type is the decoding type
*/
const char *RFM69HCW::receive(EncodingType type)
{
    if (this->avail)
    {
        this->avail = false;
        if (!decode(this->msg, type, this->msgLen)) // Note: this->msgLen is never set to -1
            return 0;
        return this->msg;
    }
    return 0;
}

/*
Returns true if a there is a new message
*/
bool RFM69HCW::available()
{
    rx();
    return this->avail;
}

/*
Returns the RSSI of the last message
*/
int RFM69HCW::RSSI()
{
    return this->rssi;
}

// probably broken
void RFM69HCW::set300KBPS()
{
    this->radio.spiWrite(0x03, 0x00);       // REG_BITRATEMSB: 300kbps (0x006B, see DS p20)
    this->radio.spiWrite(0x04, 0x6B);       // REG_BITRATELSB: 300kbps (0x006B, see DS p20)
    this->radio.spiWrite(0x19, 0x40);       // REG_RXBW: 500kHz
    this->radio.spiWrite(0x1A, 0x80);       // REG_AFCBW: 500kHz
    this->radio.spiWrite(0x05, 0x13);       // REG_FDEVMSB: 300khz (0x1333)
    this->radio.spiWrite(0x06, 0x33);       // REG_FDEVLSB: 300khz (0x1333)
    this->radio.spiWrite(0x29, 240);        // set REG_RSSITHRESH to -120dBm
    this->radio.spiWrite(0x37, 0b10010000); // DC=WHITENING, CRCAUTOOFF=0
                                            //                ^^->DC: 00=none, 01=manchester, 10=whitening
}

// utility functions
#ifndef max
int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}
#endif

#ifndef min
int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}
#endif