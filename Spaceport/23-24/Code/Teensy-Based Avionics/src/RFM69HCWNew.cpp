#include "RFM69HCWNew.h"

#ifndef max
int max(int a, int b);
#endif

#ifndef min
int min(int a, int b);
#endif

/*
Constructor
    - frequency in hertz
    - transmitter true if the radio will be on the rocket
    - highBitrate true for 300kbps, false for 4.8kbps
    - config with APRS settings
*/
RFM69HCWNew::RFM69HCWNew(const RadioSettings *s) : radio(s->cs, s->irq, *s->spi)
{

    this->settings = *s;
    this->avail = false;

    if (this->settings.transmitter)
    {
        // addresses for transmitters
        this->thisAddr = 0x02;
        this->toAddr = 0x01;
    }
    else
    {
        // addresses for receivers
        this->thisAddr = 0x01;
        this->toAddr = 0x02;
    }
}

/*
Initializer to call in setup
*/
bool RFM69HCWNew::init()
{
    // reset the radio
    pinMode(settings.rst, OUTPUT);
    digitalWrite(settings.rst, LOW);
    delay(10);
    digitalWrite(settings.rst, HIGH);
    delay(10);
    digitalWrite(settings.rst, LOW);

    if (!radio.init())
        return false;

    // then use this to actually set the frequency
    if (!radio.setFrequency(settings.frequency))
        return false;

    // set transmit power
    radio.setTxPower(txPower, true);

    // always set FSK mode
    radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);

    // set headers
    radio.setHeaderTo(toAddr);
    radio.setHeaderFrom(thisAddr);
    radio.setThisAddress(thisAddr);
    radio.setHeaderId(0);

    // configure unlimited packet length mode, don't do this for now
    // this->radio.spiWrite(0x37, 0b00000000);                // Packet format (0x37) set to 00000000 (see manual for meaning of each bit)
    // this->radio.spiWrite(RH_RF69_REG_38_PAYLOADLENGTH, 0); // Payload length (0x38) set to 0

    if (this->settings.highBitrate) // Note: not working
    {
        // the default bitrate of 4.8kbps should be fine unless we want high bitrate for video
        // set300KBPS();
        this->radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);
        // remove as much overhead as possible
        // this->radio.setPreambleLength(0);
        // this->radio.setSyncWords();
    }

    return initialized = true;
}

/*
Most basic transmission method, simply transmits the string without modification
    \param message is the message to be transmitted must be less than maxDataLen
    \param len optional length of message, required if message is not a null terminated string
*/
bool RFM69HCWNew::tx(const uint8_t *message, int len, int packetNum, bool lastPacket)
{
    // figure out how long the message should be
    if (len == -1)
    {
        len = strlen((char *)message);
    }
    radio.setHeaderId(packetNum);
    radio.setHeaderFlags(lastPacket ? 0 : RADIO_FLAG_MORE_DATA);

    uint8_t temp[555];
    memcpy(temp, message, len);
    temp[len] = '\0';
    printf("Len: %d - Message: %s\n", len, temp);

    return radio.send((uint8_t *)message, len);
}

/*
Receive function
Most basic receiving method, simply checks for a message and returns it
Note that the message may be incomplete, if the message is complete available() will return true
The received message will be truncated if it exceeds the maximum message length
\return the received message with the length in the first byte or `nullptr` if no message is available
*/
bool RFM69HCWNew::rx(uint8_t *recvbuf, uint8_t *len)
{
    if (!radio.available())
        return false;

    if (!radio.recv(recvbuf, len))
        return false;

    rssi = radio.lastRssi();
    return true;
}

#pragma region External Send and Receive

bool RFM69HCWNew::enqueueSend(const uint8_t *message, uint8_t len)
{
    // fill up the buffer with the message. buffer is a circular queue.
    buffer[bufTail] = len;
    bufTail = (bufTail + 1) % RADIO_BUFFER_SIZE;
    for (int i = 0; i < len; i++)
    {
        if (bufTail == bufHead) // buffer is full
            return false;

        buffer[bufTail] = message[i];
        bufTail = (bufTail + 1) % RADIO_BUFFER_SIZE;
    }
    printf("Length: %d\n", len);
    return true;
}

bool RFM69HCWNew::enqueueSend(const char *message)
{
    return enqueueSend((uint8_t *)message, strlen(message));
}

bool RFM69HCWNew::enqueueSend(RadioMessage *message)
{
    const uint8_t *encodedMessage = message->encode();
    int messageLength = message->length();
    return enqueueSend(encodedMessage, messageLength);
}

bool RFM69HCWNew::dequeueReceive(uint8_t *message)
{
    if (bufHead == bufTail) // buffer is empty
        return false;

    // get the length of the message
    int len = buffer[bufHead];
    inc(bufHead);
    // Empty the buffer up to the length of the expected message
    for (int i = 0; i < len && bufHead != bufTail; i++)
    {
        message[i] = buffer[bufHead];
        inc(bufHead);
    }
    return true;
}

bool RFM69HCWNew::dequeueReceive(char *message)
{
    if (bufHead == bufTail)
        return false;

    // Empty the buffer up to the first null terminator
    inc(bufHead); // skip the length byte
    int i = 0;
    for (; buffer[bufHead] != '\0' && bufHead != bufTail; i++)
    {
        message[i] = buffer[bufHead];
        inc(bufHead);
    }
    message[i] = '\0';

    return true;
}

bool RFM69HCWNew::dequeueReceive(RadioMessage *message)
{
    if (bufHead == bufTail)
        return false;

    uint8_t len = buffer[bufHead];
    uint8_t *msg = new uint8_t[len];
    if (!dequeueReceive(msg))
        return false;

    message->decode(msg, len);
    delete[] msg;
    return true;
}

#pragma endregion

bool RFM69HCWNew::update()
{
    if (busy()) // radio is busy, cannot send or receive
        return false;
    if (settings.transmitter)
    {
        if (bufHead == bufTail)
        { // buffer is empty, nothing to send
            remainingLength = 0;
            return false;
        }
        // send the message
        if (remainingLength == 0) // start a new message
        {
            remainingLength = buffer[bufHead];
            int len = min(remainingLength, maxDataLen);
            inc(bufHead);
            tx(&buffer[bufHead], len, 0, remainingLength <= maxDataLen);
            bufHead += len % RADIO_BUFFER_SIZE;
            remainingLength -= len;
        }
        else // continue the message
        {
            int len = min(remainingLength, maxDataLen);
            remainingLength -= len;
            tx(&buffer[bufHead], len, radio.headerId() + 1, remainingLength <= maxDataLen);
            bufHead += len % RADIO_BUFFER_SIZE;
        }
    }
    else // receiver
    {
        uint8_t msg[RH_RF69_MAX_MESSAGE_LEN];
        uint8_t len = RH_RF69_MAX_MESSAGE_LEN;
        if (!rx(msg, &len))
            return false;
        // store the message in the buffer
        if (radio.headerId() == 0)
        { // start of a new message
            printf("new message\n");
            orignalBufferTail = bufTail;
            buffer[bufTail] = len; // store the length of the message
            inc(bufTail);
            for (int i = 1; i <= len; i++)
            {
                buffer[bufTail] = msg[i - 1];
                inc(bufTail);
            }
        }
        else
        {
            buffer[orignalBufferTail] += len; // update the length of the message
            for (int i = 1; i <= len; i++)
            {
                buffer[bufTail] = msg[i - 1];
                inc(bufTail);
            }
        }
    }
    return true;
}

#pragma region Helpers

/*
\return `true` if the radio is currently transmitting or receiving a message
*/
bool RFM69HCWNew::busy()
{
    RHGenericDriver::RHMode mode = this->radio.mode();
    if (mode == RHGenericDriver::RHModeTx || mode == RHGenericDriver::RHModeRx)
        return true;
    return false;
}
/*
Returns the RSSI of the last message
*/
int RFM69HCWNew::RSSI()
{
    return rssi;
}

// probably broken
void RFM69HCWNew::set300KBPSMode()
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

#pragma endregion
