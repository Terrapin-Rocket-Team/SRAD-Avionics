#include "RFM69HCW.h"

/*
Constructor
    - frequency in hertz
    - transmitter true if the radio will be on the rocket
    - highBitrate true for 300kbps, false for 4.8kbps
    - config with APRS settings
*/
RFM69HCW::RFM69HCW(uint32_t frquency, bool transmitter, bool highBitrate, APRSConfig config)
{
    this->frq = frq;

    this->isTransmitter = transmitter;
    if (this->isTransmitter)
    {
        // addresses for transmitters
        this->addr = 0x0002; // to fit the max number of bytes in a packet, this will change
                             // based on message length, but it will be reset to this value
                             // if the transmitter needs to receive information from the ground
        this->toAddr = 0x0001;
    }
    else
    {
        // addresses for receivers
        this->addr = 0x0001;
        this->toAddr = 0x0002;
    }

    this->isHighBitrate = highBitrate;

    this->cfg = config;

    // avoid errors if lastMsg isn't allocated
    this->lastMsg = new char[1];
}

RFM69HCW::~RFM69HCW()
{
    delete[] this->lastMsg;
}

/*
Use the other begin function
*/
void RFM69HCW::begin()
{
#if defined(TEENSYDUINO)
    // default SPI for teensy 4.1 uses pin 10 for CS and can use pin 9 for interrupt
    this->radio = RFM69(10, 9, true, &SPI);
    this->radio = RFM69();
#else
    // attempt to create the radio, but this probably won't work
    this->radio = RFM69();
#endif

    // the frequency the initialize function uses is kinda useless, it only lets you choose from preselected frequencies
    // so set the frequency to the right band 433 or 915 Mhz at first
    radio.initialize(this->frq < 914e6 ? RF69_433MHZ : RF69_915MHZ, this->addr, this->networkID);
    // then use this to actually set the frequency
    radio.setFrequency(this->frq);
    radio.setHighPower(true);
    radio.setPowerDBm(this->txPower);
    radio.encrypt(0);
    // the default bitrate of 4.8kbps should be fine unless we want high bitrate for video
    if (this->isHighBitrate)
        radio.set300KBPS();
}

/*
Initializer to call in setup
    - s is the spi interface that should be used
    - cs is the chip select pin
    - irq is the interrupt pin
*/
void RFM69HCW::begin(SPIClass *s, uint8_t cs, uint8_t irq)
{

    this->radio = RFM69(cs, irq, true, s);

    // the frequency the initialize function uses is kinda useless, it only lets you choose from preselected frequencies
    // so set the frequency to the right band 433 or 915 Mhz at first
    radio.initialize(this->frq < 914e6 ? RF69_433MHZ : RF69_915MHZ, this->addr, this->networkID);
    // then use this to actually set the frequency
    radio.setFrequency(this->frq);
    radio.setHighPower(true);
    radio.setPowerDBm(this->txPower);
    radio.encrypt(0);
    // the default bitrate of 4.8kbps should be fine unless we want high bitrate for video
    if (this->isHighBitrate)
        radio.set300KBPS();
}

/*
Transmit function
Most basic transmission method, simply transmits the string without modification
    - message is the message to be transmitted, must be null terminated
*/
bool RFM69HCW::tx(char *message)
{
    // get length because message may be too long to transmit at one time
    int len = strlen(message);

    // get number of packets for this message, and set this radio's address to that value so the receiver knows how many packets to expect
    this->addr = len / bufSize + (len % bufSize > 0);
    radio.setAddress(this->addr);

    // fill the buffer repeatedly until the entire message has been sent
    for (int j = 0; j < this->addr; j++)
    {
        memcpy(this->buf, message + j * 61, min(61, len - j * 61));

        radio.send(this->toAddr, (void *)buf, this->bufSize, false);
    }

    this->addr = 0x0002;
    radio.setAddress(this->addr);

    return true;
}

/*
Receive function
Most basic receiving method, simply checks for a message and returns a pointer to it
Note that the message may be incomplete, if the message is complete available() will return true
*/
const char *RFM69HCW::rx()
{
    if (radio.receiveDone())
    {
        // copy data from the radio object
        strncpy(this->buf, (char *)radio.DATA, bufSize);

        // check if this is the first part of the message
        if (this->incomingMsgLen == 0)
        {
            this->incomingMsgLen = radio.SENDERID;
            delete[] this->lastMsg;
            this->lastMsg = new char[this->incomingMsgLen * bufSize + 1];
            memcpy(this->lastMsg, this->buf, bufSize);
            this->lastMsg[bufSize] = '\0';
        }
        else // otherwise append to the end of the message, removing the \0 from last time
        {
            int len = strlen(this->lastMsg);
            memcpy(this->lastMsg + len, this->buf, bufSize);
            this->lastMsg[len + bufSize] = '\0';
        }

        // check if the full message has been received
        int msgLen = strlen(this->lastMsg);
        if (msgLen / bufSize + (msgLen % bufSize > 0) == this->incomingMsgLen)
        {
            this->incomingMsgLen = 0;
            this->avail = true;
        }
        this->avgRSSI += radio.readRSSI();
        return this->lastMsg;
    }
    return "";
}

/*
Multi-purpose encoder function
Encodes the message into a format selected by type
char *message must be a dynamically allocated null terminated string
- Telemetry:
    message - input: latitude,longitude,altitude,speed,heading,precision,stage,t0 -> output: APRS message
- Video: TODO
    message - input: Base 64 string -> output: ?
- Ground Station: TODO
    message - input: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value -> output: APRS message
*/
bool RFM69HCW::encode(char *message, EncodingType type)
{
    if (type == ENCT_TELEMETRY)
    {
        APRSMsg aprs;
        int msgLen = strlen(message);
        // make sure message has enough space for the output
        if (msgLen < 110)
        {
            char *msg = new char[msgLen];
            strcpy(msg, message);
            delete[] message;
            message = new char[111];
            strcpy(message, msg);
            delete[] msg;
        }

        aprs.setSource(this->cfg.CALLSIGN);
        aprs.setPath(this->cfg.PATH);
        aprs.setDestination(this->cfg.TOCALL);

        // holds the data to be assembled into the aprs body
        APRSData data;

        // find each value separated in order by a comma and put in the APRSData array
        char *currentVal = new char[msgLen];
        int currentValIndex = 0;
        int currentValCount = 0;
        for (int i = 0; i < msgLen; i++)
        {
            if (message[i] != ',')
            {
                currentVal[currentValIndex] = message[i];
                currentValIndex++;
            }
            if (message[i] == ',')
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

        // get lat and long string for low or high precision
        if (data.precision == 'L')
        {
            strcpy(data.dao, "");
            create_lat_aprs(&data.lat, 0);
            create_long_aprs(&data.lng, 0);
        }
        else if (data.precision == 'H')
        {
            strcpy(data.dao, create_dao_aprs(data.lat, data.lng));
            create_lat_aprs(&data.lat, 1);
            create_long_aprs(&data.lng, 1);
        }

        // get alt string
        int alt_int = max(-99999, min(999999, atoi(data.alt)));
        if (alt_int < 0)
        {
            strcpy(data.alt, "/A=-");
            padding(alt_int * -1, 5, &data.alt, 4);
        }
        else
        {
            strcpy(data.alt, "/A=");
            padding(alt_int, 6, &data.alt, 3);
        }

        // get course/speed strings
        // TODO add speed zero counter (makes decoding more complex)
        int spd_int = max(0, min(999, atoi(data.spd)));
        int hdg_int = max(0, min(360, atoi(data.hdg)));
        if (hdg_int == 0)
            hdg_int = 360;
        padding(spd_int, 3, &data.spd);
        padding(hdg_int, 3, &data.hdg);

        // generate the aprs message
        char body[80];
        sprintf(body, "%c%s%c%s%c%s%c%s%s%s%s%c%s%c%s", '!', data.lat, this->cfg.OVERLAY, data.lng, this->cfg.SYMBOL,
                data.hdg, '/', data.spd, data.alt, "/S", data.stage, '/',
                data.t0, ' ', data.dao);
        aprs.getBody()->setData(body);
        aprs.encode(message);
        return true;
    }
    return false;
}

/*
Multi-purpose encoder function
Decodes the message into a format selected by type
char *message must be a dynamically allocated null terminated string
- Telemetry: TODO
    message - input: APRS message -> output: latitude,longitude,altitude,speed,heading,precision,stage,t0
- Video: TODO
    message - input: Base 64 string -> output: ?
- Ground Station:
    message - input: APRS message -> output: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value
*/
bool RFM69HCW::decode(char *message, EncodingType type)
{
    if (type == ENCT_TELEMETRY)
    {
        APRSMsg aprs;
        aprs.decode(message);
        // make sure message has enough space for the output
        if (strlen(message) < 70)
        {
            delete[] message;
            message = new char[71];
        }
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
        data.lat[i - (bodyptr - body)] = '\0';
        i++;
        bodyptr = body + i;

        // find longitude
        while (body[i] != '[' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.lng, bodyptr, i - (bodyptr - body));
        data.lng[i - (bodyptr - body)] = '\0';
        i++;
        bodyptr = body + i;

        // find heading
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.hdg, bodyptr, i - (bodyptr - body));
        data.hdg[i - (bodyptr - body)] = '\0';
        i++;
        bodyptr = body + i;

        // find speed
        while (body[i] != '/' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.spd, bodyptr, i - (bodyptr - body));
        data.spd[i - (bodyptr - body)] = '\0';
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
        data.alt[i - (bodyptr - body)] = '\0';
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
        data.stage[i - (bodyptr - body)] = '\0';
        i++;
        bodyptr = body + i;

        // find t0
        while (body[i] != ' ' && i != len)
            i++;
        if (i == len)
            return false;
        strncpy(data.t0, bodyptr, i - (bodyptr - body));
        data.t0[i - (bodyptr - body)] = '\0';
        i++;
        bodyptr = body + i;

        // find dao
        strncpy(data.dao, bodyptr, len - (bodyptr - body));
        data.dao[len - (bodyptr - body)] = '\0';

        // TODO convert lat and lng to degrees

        sprintf(message, "%s,%s,%s,%s,%s,%s,%s,%s", data.lat, data.lng, data.alt, data.spd, data.hdg, strlen(data.dao) > 0 ? 'H' : 'L', data.stage, data.t0);

        return true;
    }
    if (type == ENCT_GROUNDSTATION)
    {
        // put the message into a APRSMessage object to decode it
        APRSMsg aprs;
        aprs.decode(message);
        // make sure message has enough space for the output
        if (strlen(message) < 180)
        {
            delete[] message;
            message = new char[181];
        }
        aprs.toString(message);
        // add RSSI to the end of message
        sprintf(message + strlen(message), "%s%d", ",RSSI:", this->avgRSSI / this->incomingMsgLen);
        return true;
    }
    return false;
}

/*
Comprehensive send function
Encodes the message into the selected type, then sends it
char *message must be a null terminated string
    - message is the message to be sent
    - type is the encoding type
*/
bool RFM69HCW::send(char *message, EncodingType type)
{
    delete[] this->lastMsg;
    this->lastMsg = new char[strlen(message) + 1];
    strcpy(this->lastMsg, message);

    return encode(this->lastMsg, type) && tx(this->lastMsg);
}

/*
Comprehensive receive function
Should be called after verifying there is an available message by calling available()
Decodes the last received message according to the type
    - type is the decoding type
*/
const char *RFM69HCW::receive(EncodingType type)
{
    if (this->avail)
    {
        this->avail = false;
        decode(this->lastMsg, type);
        return this->lastMsg;
    }
    return "";
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
    return this->avgRSSI / this->incomingMsgLen;
}

// utility functions
// <algorithm> header has a max and min function - danny
int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

int min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}