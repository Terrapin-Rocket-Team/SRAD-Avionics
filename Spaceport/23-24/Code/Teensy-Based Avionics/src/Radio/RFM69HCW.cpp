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
    radio.setTxPower(settings.txPower, true);

    // always set FSK mode
    radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);

    // set headers
    radio.setHeaderTo(settings.toAddr);
    radio.setHeaderFrom(settings.thisAddr);
    radio.setThisAddress(settings.thisAddr);
    radio.setHeaderId(0);

    // configure unlimited packet length mode, don't do this for now
    // this->radio.spiWrite(0x37, 0b00000000);                // Packet format (0x37) set to 00000000 (see manual for meaning of each bit)
    // this->radio.spiWrite(RH_RF69_REG_38_PAYLOADLENGTH, 0); // Payload length (0x38) set to 0

    // if (this->settings.highBitrate) // Note: not working
    // {
    //     // the default bitrate of 4.8kbps should be fine unless we want high bitrate for video
    //     // set300KBPS();
    //     this->radio.setModemConfig(RH_RF69::FSK_Rb4_8Fd9_6);
    //     // remove as much overhead as possible
    //     // this->radio.setPreambleLength(0);
    //     // this->radio.setSyncWords();
    // }

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
    int originalTail = sendBuffer.tail;
    sendBuffer.data[sendBuffer.tail] = len; // store the length of the message
    inc(sendBuffer.tail);

    for (int i = 0; i < len; i++) // same algorithm as copyToBuffer but with a length check
    {
        if (sendBuffer.tail == sendBuffer.head) // buffer is full
        {
            sendBuffer.tail = originalTail; // reset the tail (no message was added)
            return false;
        }

        sendBuffer.data[sendBuffer.tail] = message[i];
        inc(sendBuffer.tail);
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
    if (message->encode())
        return enqueueSend(message->getArr(), message->length());
    return false;
}
/*
Dequeue a message from the buffer and decode it into a uint8_t[].
    \param message the uint8_t[] to copy the recieved message into
    \return `true` if a message was copied, `false` otherwise
*/
bool RFM69HCW::dequeueReceive(uint8_t *message)
{
    if (recvBuffer.head == recvBuffer.tail) // buffer is empty
        return false;

    // get the length of the message
    int len = recvBuffer.data[recvBuffer.head];
    inc(recvBuffer.head);
    // Empty the buffer up to the length of the expected message
    copyFromBuffer(recvBuffer, message, len);
    return true;
}

/*
Dequeue a message from the buffer and decode it into a char[]. WIll break if the recieved message is not a null terminated string.
    \param message the char[] to copy the recieved message into
    \return `true` if a message was copied, `false` otherwise
*/
bool RFM69HCW::dequeueReceive(char *message)
{
    if (recvBuffer.head == recvBuffer.tail) // buffer is empty
        return false;

    // Empty the buffer up to the first null terminator
    inc(recvBuffer.head); // skip the length byte, should be null terminated
    int i = 0;
    for (; recvBuffer.data[recvBuffer.head] != '\0' && recvBuffer.head != recvBuffer.tail; i++)
    {
        message[i] = recvBuffer.data[recvBuffer.head];
        inc(recvBuffer.head);
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
    if (recvBuffer.head == recvBuffer.tail) // buffer is empty
        return false;

    uint8_t len = recvBuffer.data[recvBuffer.head];
    uint8_t *msg = new uint8_t[len];
    bool worked = false;
    if (dequeueReceive(msg))           // get the message from the buffer
        if (message->setArr(msg, len)) // send the message to the RadioMessage
            if (message->decode())     // decode the message
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
    if (!initialized)
        return false;
    if (busy())
        return false;

    uint8_t rcvLen = RH_RF69_MAX_MESSAGE_LEN;
    uint8_t rcvBuf[RH_RF69_MAX_MESSAGE_LEN];

    if (rx(rcvBuf, &rcvLen))
    {
        // store the message in the buffer
        if (radio.headerId() == 0) // start of a new message
        {
            orignalBufferTail = recvBuffer.tail;
            recvBuffer.data[recvBuffer.tail] = rcvLen; // store the length of the message
            inc(recvBuffer.tail);
        }
        else                                              // continuing message
            recvBuffer.data[orignalBufferTail] += rcvLen; // update the total length of the message

        copyToBuffer(recvBuffer, rcvBuf, rcvLen);
        if (radio.headerFlags() & RADIO_FLAG_MORE_DATA) // more data is coming, should not process yet
            return false;
        return true;
    }

    // transmit
    if (sendBuffer.head == sendBuffer.tail) // buffer is empty, nothing to send
    {
        remainingLength = 0;
        return false;
    }

    int packetNum;
    if (remainingLength == 0) // start a new message
    {
        remainingLength = sendBuffer.data[sendBuffer.head]; // get the length of the message from the first byte of the message
        inc(sendBuffer.head);
        packetNum = 0;
    }
    else
        packetNum = radio.headerId() + 1;

    // load message to tx
    int len = min(remainingLength, RH_RF69_MAX_MESSAGE_LEN); // how much of the message to send
    uint8_t msgToTransmit[RH_RF69_MAX_MESSAGE_LEN];
    copyFromBuffer(sendBuffer, msgToTransmit, len);
    tx(msgToTransmit, len, packetNum, remainingLength <= RH_RF69_MAX_MESSAGE_LEN);
    remainingLength -= len;
    return false; // always false for transmitter
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
// void RFM69HCW::set300KBPSMode()
// {
//     this->radio.spiWrite(0x03, 0x00);       // REG_BITRATEMSB: 300kbps (0x006B, see DS p20)
//     this->radio.spiWrite(0x04, 0x6B);       // REG_BITRATELSB: 300kbps (0x006B, see DS p20)
//     this->radio.spiWrite(0x19, 0x40);       // REG_RXBW: 500kHz
//     this->radio.spiWrite(0x1A, 0x80);       // REG_AFCBW: 500kHz
//     this->radio.spiWrite(0x05, 0x13);       // REG_FDEVMSB: 300khz (0x1333)
//     this->radio.spiWrite(0x06, 0x33);       // REG_FDEVLSB: 300khz (0x1333)
//     this->radio.spiWrite(0x29, 240);        // set REG_RSSITHRESH to -120dBm
//     this->radio.spiWrite(0x37, 0b10010000); // DC=WHITENING, CRCAUTOOFF=0
//                                             //                ^^->DC: 00=none, 01=manchester, 10=whitening
// }

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

void RFM69HCW::copyToBuffer(buffer &buffer, const uint8_t *src, int len)
{
    for (int i = 0; i < len; i++)
    {
        buffer.data[buffer.tail] = src[i];
        inc(buffer.tail);
    }
}
// pop is true by default. If pop is false, the buffer head will not be moved.
void RFM69HCW::copyFromBuffer(buffer &buffer, uint8_t *dest, int len, bool pop)
{
    int originalHead = buffer.head;
    for (int i = 0; i < len; i++)
    {
        dest[i] = buffer.data[buffer.head];
        inc(buffer.head);
    }
    if (!pop)
        buffer.head = originalHead;
}

#pragma endregion
