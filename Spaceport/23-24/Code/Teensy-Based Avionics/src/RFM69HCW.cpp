#include "RFM69HCW.h"

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
RFM69HCW::RFM69HCW(const RadioSettings *s) : radio(s->cs, s->irq, *s->spi)
{

    this->settings = *s;

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
bool RFM69HCW::init()
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
    \param message is the message to be transmitted, must be shorter than RH_RF69_MAX_MESSAGE_LEN
    \param len optional length of message, required if message is not a null terminated string
    \param packetNum optional packet number of the message
    \param lastPacket optional is this the last packet of a larger message or the only packet?
    \return `true` if the message was sent, `false` otherwise
*/
bool RFM69HCW::tx(const uint8_t *message, int len, int packetNum, bool lastPacket)
{
    // figure out how long the message should be
    if (len == -1)
        len = strlen((char *)message);
    
    radio.setHeaderId(packetNum);
    radio.setHeaderFlags(lastPacket ? 0 : RADIO_FLAG_MORE_DATA);

    return radio.send((uint8_t *)message, len);
}

/*
Receive function -
Most basic receiving method, simply checks for a message and returns it
Note that the message may be incomplete, if the message is complete update() will return true
The received message will be truncated if it exceeds the maximum message length of the radio. Also updates the RSSI.
    \param recvbuf the buffer to store the received message
    \param len the length of the received message (updated by the function)
    \return `true` if a message was received, `false` otherwise
*/
bool RFM69HCW::rx(uint8_t *recvbuf, uint8_t *len)
{
    if (!radio.recv(recvbuf, len))
        return false;

    rssi = radio.lastRssi();
    return true;
}

#pragma region External Send and Receive

/*
Enqueue a message into the buffer
    \param message the uint8_t[] to enqueue
    \param len the length of the message
    \return `true` if the message was enqueued, `false` otherwise
*/
bool RFM69HCW::enqueueSend(const uint8_t *message, uint8_t len)
{
    // fill up the buffer with the message. buffer is a circular queue.
    int originalTail = bufTail;
    buffer[bufTail] = len;
    inc(bufTail);

    for (int i = 0; i < len; i++) // same algorithm as copyToBuffer but with a length check
    {
        if (bufTail == bufHead)
        {                           // buffer is full
            bufTail = originalTail; // reset the tail (no message was added)
            return false;
        }

        buffer[bufTail] = message[i];
        inc(bufTail);
    }
    return true;
}

/*
Enqueue a message into the buffer
    \param message the null terminated string to enqueue
    \return `true` if the message was enqueued, `false` otherwise
*/
bool RFM69HCW::enqueueSend(const char *message)
{
    return enqueueSend((uint8_t *)message, strlen(message));
}
/*
Encode the message and enqueue it in the buffer
    \param message the RadioMessage to encode and enqueue
    \return `true` if the message was encoded and enqueued, `false` otherwise
*/
bool RFM69HCW::enqueueSend(RadioMessage *message)
{
    uint8_t encodedMessage[RadioMessage::MAX_MESSAGE_LEN];
    if (message->encode(encodedMessage))
    {
        int messageLength = message->length();
        return enqueueSend(encodedMessage, messageLength);
    }
    return false;
}
/*
Dequeue a message from the buffer and decode it into a uint8_t[].
    \param message the uint8_t[] to copy the recieved message into
    \return `true` if a message was copied, `false` otherwise
*/
bool RFM69HCW::dequeueReceive(uint8_t *message)
{
    if (bufHead == bufTail) // buffer is empty
        return false;

    // get the length of the message
    int len = buffer[bufHead];
    inc(bufHead);
    // Empty the buffer up to the length of the expected message
    copyFromBuffer(message, len);
    return true;
}

/*
Dequeue a message from the buffer and decode it into a char[]. WIll break if the recieved message is not a null terminated string.
    \param message the char[] to copy the recieved message into
    \return `true` if a message was copied, `false` otherwise
*/
bool RFM69HCW::dequeueReceive(char *message)
{
    if (bufHead == bufTail) // buffer is empty
        return false;

    // Empty the buffer up to the first null terminator
    inc(bufHead); // skip the length byte, should be null terminated
    int i = 0;
    for (; buffer[bufHead] != '\0' && bufHead != bufTail; i++)
    {
        message[i] = buffer[bufHead];
        inc(bufHead);
    }
    message[i] = '\0';

    return true;
}

/*
Dequeue a message from the buffer and decode it into a RadioMessage
    \param message the RadioMessage to give the encoded text to for decoding
    \return `true` if a message was decoded, `false` if no message was decoded
*/
bool RFM69HCW::dequeueReceive(RadioMessage *message)
{
    if (bufHead == bufTail) // buffer is empty
        return false;

    uint8_t len = buffer[bufHead];
    uint8_t *msg = new uint8_t[len];
    bool worked = false;
    if (dequeueReceive(msg))           // get the message from the buffer
        if (message->decode(msg, len)) // decode the message
            worked = true;

    delete[] msg;
    return worked;
}

#pragma endregion

/*
Update function
    - checks if the radio is busy
    - if the radio is a transmitter, sends (partially sends, if it's too long) the next queued message in the buffer
    - if the radio is a receiver, receives the next message and stores it in the buffer
    \return TRANSMITTER: `true` if the message was sent, `false` if the message was not sent.
    \return RECIEVER: `true` if the message was received in full, `false` if nothing received or message is incomplete.
*/
bool RFM69HCW::update()
{
    if (busy()) // radio is busy, cannot send or receive
        return false;

    // transmitter------------------------------------------------------
    if (settings.transmitter)
    {
        if (bufHead == bufTail) // buffer is empty, nothing to send
        {
            remainingLength = 0;
            return false;
        }

        // send the message
        int packetNum;
        if (remainingLength == 0) // start a new message
        {
            remainingLength = buffer[bufHead]; // get the length of the message from the first byte of the message
            inc(bufHead);
            packetNum = 0;
        }
        else
            packetNum = radio.headerId() + 1;

        // load message to tx
        int len = min(remainingLength, RH_RF69_MAX_MESSAGE_LEN); // how much of the message to send
        uint8_t msgToTransmit[RH_RF69_MAX_MESSAGE_LEN];
        copyFromBuffer(msgToTransmit, len);
        bool b = tx(msgToTransmit, len, packetNum, remainingLength <= RH_RF69_MAX_MESSAGE_LEN);
        remainingLength -= len;
        return b;
    }
    // receiver------------------------------------------------------
    else
    {
        uint8_t msg[RH_RF69_MAX_MESSAGE_LEN];
        uint8_t len = RH_RF69_MAX_MESSAGE_LEN;
        if (!rx(msg, &len))
            return false; // no new message available

        // store the message in the buffer
        if (radio.headerId() == 0) // start of a new message
        {
            orignalBufferTail = bufTail;
            buffer[bufTail] = len; // store the length of the message
            inc(bufTail);
        }
        else                                  // continuing message
            buffer[orignalBufferTail] += len; // update the total length of the message

        copyToBuffer(msg, len);
        if (radio.headerFlags() & RADIO_FLAG_MORE_DATA) // more data is coming, should not process yet
            return false;
    }
    return true; // message was sent or received (in full)
}

#pragma region Helpers

/*
\return `true` if the radio is currently transmitting or receiving a message
*/
bool RFM69HCW::busy()
{
    RHGenericDriver::RHMode mode = this->radio.mode();
    if (mode == RHGenericDriver::RHModeTx)
        return true;
    return false;
}
/*
Returns the RSSI of the last message
*/
int RFM69HCW::RSSI()
{
    return rssi;
}

// probably broken
void RFM69HCW::set300KBPSMode()
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

void RFM69HCW::copyToBuffer(const uint8_t *message, int len)
{
    for (int i = 0; i < len; i++)
    {
        buffer[bufTail] = message[i];
        inc(bufTail);
    }
}

void RFM69HCW::copyFromBuffer(uint8_t *message, int len)
{
    for (int i = 0; i < len; i++)
    {
        message[i] = buffer[bufHead];
        inc(bufHead);
    }
}

#pragma endregion
