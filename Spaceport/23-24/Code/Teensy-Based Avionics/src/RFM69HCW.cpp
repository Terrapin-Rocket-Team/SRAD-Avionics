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
        this->addr = 0x0000;
        this->toAddr = 0x0001;
    }
    else
    {
        // addresses for receivers
        this->addr = 0x0001;
        this->toAddr = 0x0000;
    }

    this->isHighBitrate = highBitrate;

    this->cfg = config;
    // TODO intialize lastMsg maybe in custom APRS class
}

/*
Initializer to call in setup
    - s is the spi interface that should be used
    - cs is the chip select pin
    - irq is the interrupt pin
    - frqBand is 915 or 433 depending on radio
*/
void RFM69HCW::begin(SPIClass *s, uint8_t cs, uint8_t irq, int frqBand)
{

    this->radio = RFM69(cs, irq, true, s);

    // the frequency the initialize function uses is kinda useless, it only lets you choose from preselected frequencies
    // so set the frequency to the right band 433 or 915 Mhz at first
    radio.initialize(frqBand, this->addr, this->networkID);
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
    - message is the message to be transmitted
*/
bool RFM69HCW::tx(char *message)
{
    // TODO may need to append start and end byte to each message so we know when it ends
    // create a temporary buffer with the full string, which may (possibly?) be too long to transmit at one time
    int len = strlen(message);

    int pos = 0;
    int i = 0;
    // fill the buffer repeatedly until the entire message has been looped over
    // note i will always be 1 more than the last index after i++
    while (pos < len)
    {
        this->buf[i] = message[pos];
        i++;
        pos++;
        // send a transmission each time the buffer is filled
        if (i == this->bufSize)
        {
            radio.send(this->toAddr, (void *)buf, this->bufSize, false);
            i = 0;
        }
    }
    // send any remaining data
    if (i > 0)
        radio.send(this->toAddr, (void *)buf, i, false);
    return true;
}

/*
Receive function
Most basic receiving method, simply checks for a message and returns it
*/
const char *RFM69HCW::rx()
{
    // TODO may need to check for start and end byte for each message
    if (radio.receiveDone())
    {
        this->avail = true;
        this->lastMsg = (char *)radio.DATA;
        this->lastRSSI = radio.readRSSI();
        return this->lastMsg;
    }
    return "";
}

/*
Multi-purpose encoder function
Encodes the message into a format selected by type
- Telemetry:
    message - input: latitude,longitude,altitude,speed,heading,precision,stage,t0 -> output: APRS message
- Video: TODO
    message - input: Base 64 string -> output: ?
- Ground Station: TODO
    message - input: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value -> output: APRS message
*/
bool RFM69HCW::encode(char **message, EncodingType type)
{
    if (type == ENCT_TELEMETRY)
    {
        APRSMsg aprs;
        int msgLen = strlen(*message);

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
            if ((*message)[i] != ',')
            {
                currentVal[currentValIndex] = (*message)[i];
                currentValIndex++;
            }
            if ((*message)[i] == ',')
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
        int spd_int = max(0, min(999, atoi(data.spd)));
        int hdg_int = max(0, min(360, atoi(data.hdg)));
        if (hdg_int == 0)
            hdg_int = 360;
        padding(spd_int, 3, &data.spd);
        padding(hdg_int, 3, &data.hdg);

        // generate the aprs message
        char body[60];
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
- Telemetry: TODO
    message - input: APRS message -> output: latitude,longitude,altitude,speed,heading,precision,stage,t0
- Video: TODO
    message - input: Base 64 string -> output: ?
- Ground Station:
    message - input: APRS message -> output: Source:Value,Destination:Value,Path:Value,Type:Value,Body:Value
*/
bool RFM69HCW::decode(char **message, EncodingType type)
{
    if (type == ENCT_TELEMETRY)
    {
        APRSMsg aprs;
        aprs.decode(*message);
        // String body = aprs.getBody()->getData();
        //  TODO
    }
    if (type == ENCT_GROUNDSTATION)
    {
        // TODO will be fixed by custom aprs decoder/encoder
        // put the message into a APRSMessage object to decode it
        APRSMsg aprs;
        aprs.decode(*message);
        // message = aprs.toString().replace(" ", "");
        //  add RSSI to the end of message
        sprintf(*message + strlen(*message), "%s%d", ",RSSI:", this->lastRSSI);
        return true;
    }
    return false;
}

/*
Comprehensive send function
Encodes the message into the selected type, then sends it
    - message is the message to be sent
    - type is the encoding type
*/
bool RFM69HCW::send(char *message, EncodingType type)
{
    return encode(&message, type) && tx(message);
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
        decode(&(this->lastMsg), type);
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
    return this->lastRSSI;
}

// utility functions

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