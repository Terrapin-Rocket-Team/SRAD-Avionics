#include "MockRadio.h"

MockRadio::MockRadio()
{
    this->s = (HardwareSerial *)&Serial;
    this->dataRate = 9600;
}

MockRadio::MockRadio(MockHardwareConfig hConfig, MockPinConfig pConfig)
{
    this->s = pConfig.s;
    this->dataRate = hConfig.dataRate;
}

MockRadio::~MockRadio() {}

bool MockRadio::begin()
{
    this->s->begin(this->dataRate);

    // clear serial buffer
    // this is stupid but we need to clear a random null character that shows up when the serial port is connected
    // but not immediately after serial.begin
    // so wait a second and then completely flush the input buffer
    delay(1000);
    this->s->flush();
    while (this->s->available() > 0)
        this->s->read();

    return true;
}

bool MockRadio::tx(const uint8_t *message, int len)
{
    // make sure the packet isn't too long
    if (len > MockRadio::MAX_LEN)
        return false; // Error: the packet is too long

    // Serial.println(this->state);
    //  prefill fifo in idle state
    if (this->state == STATE_IDLE || this->state == STATE_RX || this->state == STATE_RX_COMPLETE)
    {
        // Serial.println("tx");
        // add the message to the internal buffer
        this->length = len;
        this->availLen = len;
        this->xfrd = 0;
        memcpy(this->buf, message, this->length);
        // reset available since we have just overwritten the internal buffer
        this->available = false;

        //  enter idle state

        // clear fifo
        this->bufClear();

        // start spi

        // write to TX FIFO

        // send length
        uint8_t mLen[2] = {0};
        to_bytes(this->length, 0, 0, mLen);
        this->bufWrite(mLen[0]);
        this->bufWrite(mLen[1]);

        // send message body
        int count = 0;
        while (count++ < FIFO_LENGTH - 2 && this->xfrd < this->length)
        {
            this->bufWrite(this->buf[this->xfrd++]);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        // set packet length for variable length packets

        // start tx
        // enter rx state after tx
        this->setTXMode();
        this->state = STATE_TX;

        return true;
    }
    return false;
}

void MockRadio::handleTX()
{
    // Serial.println("handleTX1");
    // Serial.println(this->xfrd);
    // Serial.println(this->length);
    // this function assumes we are in tx mode already, so check that we are in tx mode
    // availLen is the same as length in static TX
    if (!this->TXEmptyFlag && this->xfrd < this->availLen && this->gpio0())
    {
        this->TXEmptyFlag = true;
        // Serial.println("handleTX");
        // Serial.println(this->xfrd);
        // Serial.println(this->availLen);

        // write to the TX FIFO

        // write remaining data
        int count = 0;
        while (count++ < TX_THRESH && this->xfrd < this->availLen)
        {
            this->bufWrite(this->buf[this->xfrd++]);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();
    }
    // if we've sent this->length bytes, the message is complete
    if (this->xfrd == this->length)
    {
        // automatically placed into an idle state
        this->state = STATE_TX_COMPLETE;
        // clear internal variables
        memset(this->buf, 0, this->length);
        this->length = 0;
        this->availLen = 0;
        this->xfrd = 0;
    }
}

bool MockRadio::rx()
{
    // make sure we aren't already in RX mode
    if (this->state == STATE_IDLE)
    {
        // reset availLen
        this->availLen = 0;
        this->length = 0; // need to reset length, info in buf now lost

        // enter idle state

        // clear fifo
        this->bufClear();

        // set back to max length for rx mode?

        // enter RX mode
        this->setRXMode();
        this->state = STATE_RX;
        return true;
    }
    return false;
}

void MockRadio::handleRX()
{
    // uint8_t cClearFIFO[1] = {0b00000000};
    // uint8_t rClearFIFO[2] = {0x00, 0x00};
    // sendCommand(C_FIFO_INFO, 1, cClearFIFO, 2, rClearFIFO);
    // Serial.println("FIFO STATUS");
    // for (int i = 0; i < sizeof(rClearFIFO); i++)
    //     Serial.println(rClearFIFO[i]);
    // uint8_t cIntArgs[3] = {0, 0, 0};
    // uint8_t rIntArgs[8] = {};
    // sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);
    // Serial.println("INTERRUPTS");
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(rIntArgs[i], BIN);
    // }
    // assume we are in RX mode
    // this is how we read the packet until we have less than the RX FIFO THRESH left
    if (!this->RXFullFlag && this->gpio1()) // valid preamble and more than RX_THRESH bytes in FIFO
    {
        this->RXFullFlag = true;
        // this->debugTimer = micros();
        // Serial.println("here");
        // Serial.println(this->xfrd);
        // Serial.println(this->length);
        // uint8_t cClearFIFO[1] = {0b00000000};
        // uint8_t rClearFIFO[2] = {0x00, 0x00};
        // sendCommand(C_FIFO_INFO, 1, cClearFIFO, 2, rClearFIFO);
        // Serial.println("FIFO STATUS");
        // for (int i = 0; i < sizeof(rClearFIFO); i++)
        //     Serial.println(rClearFIFO[i]);

        // rssi should be available

        // read from RX FIFO

        // holds data received this iteration
        int lenBytes = 0;

        // if the internal length and xfrd variables are 0, then this is the first part of the message
        if (this->xfrd == 0)
        {
            // so we need to read length
            uint8_t mLen[2] = {0x00, 0x00};
            mLen[0] = this->bufRead();
            mLen[1] = this->bufRead();
            // convert individual bytes to uint16_t
            from_bytes(this->length, 0, 0, mLen);
            // Serial.print("len ");
            // Serial.println(this->length);
            lenBytes += 2;
            // make sure the message is not too long (could be erroneous transmission)
            if (this->length > MockRadio::MAX_LEN || this->length == 0)
            {
                this->length = 0;
                return; // error, message too long or too short
            }
        }

        // receive message data
        int count = lenBytes;
        while (this->xfrd < this->length && count < RX_THRESH)
        {
            count++;
            this->buf[this->xfrd++] = this->bufRead();
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        // Serial.print("count ");
        // Serial.print(count);
        // Serial.print("xfrd ");
        // Serial.println(this->xfrd);

        // if we've transferred length bytes, we've received the whole message
        if (this->xfrd == this->length && this->length > 0)
        {
            // Serial.println("Complete");
            // automatically placed into an idle state
            this->state = STATE_RX_COMPLETE;
            this->available = true;
            // only reset xfrd and availLen
            // length and buf need to stay so they can be read
            this->xfrd = 0;
            this->availLen = 0;
        }
    }
    if (this->length > 0 && (this->length - this->xfrd < RX_THRESH))
    {
        // Serial.println("Here2");
        // Serial.println(this->xfrd);
        // Serial.println(this->length);
        int rFIFOInfo0 = this->intBufP;
        // Serial.println("FIFO STATUS");
        // for (int i = 0; i < sizeof(rFIFOInfo); i++)
        //     Serial.println(rFIFOInfo[i]);

        // dont need to send an SPI command unless there's actually bytes to read
        if (rFIFOInfo0 > 0) // TODO: is there a better way to do this?
        {

            // read from RX FIFO

            // receive message data
            int count = 0;
            while (this->xfrd < this->length && count < rFIFOInfo0)
            {
                count++;
                this->buf[this->xfrd++] = this->bufRead();
                // Serial.print((char)this->buf[this->xfrd - 1]);
            }
            // Serial.println();
            // Serial.print("count ");
            // Serial.print(count);
            // Serial.print("\txfrd ");
            // Serial.println(this->xfrd);
        }

        // if we've transferred length bytes, we've received the whole message
        if (this->xfrd == this->length)
        {
            // Serial.println("Complete2");
            // automatically placed into an idle state
            this->state = STATE_RX_COMPLETE;
            this->available = true;
            // this->hasPacket = false;
            // only reset xfrd
            // length and buf need to stay so they can be read
            this->xfrd = 0;
        }
    }
    // if (!gpio2())
    // {
    //     Serial.println("ERROR: failed to receive packet");
    //     this->state = STATE_IDLE;
    //     this->available = false;
    // }
    // if (rIntArgs[2] & 0b00001000)
    // {
    //     Serial.println("Invalid packet! CRC failed.");
    //     this->state = STATE_IDLE;
    //     this->available = false;
    // }
}

bool MockRadio::startTX(const uint8_t *data, uint16_t len, uint16_t totalLen)
{
    // make sure the packet isn't too long and we have at least 1 byte
    if (totalLen > MockRadio::MAX_LEN && len > 0)
        return false; // Error: the packet is too long

    //  prefill fifo in idle state
    if (this->state == STATE_IDLE || this->state == STATE_RX || this->state == STATE_RX_COMPLETE)
    {
        // otherwise add the message to the internal buffer
        this->length = totalLen;
        this->availLen = len;
        this->xfrd = 0;
        memcpy(this->buf, data, this->availLen);
        // Serial.println("tx");
        //  enter idle state

        // clear fifo
        this->bufClear();

        // start spi

        // write to TX FIFO

        // send length
        uint8_t mLen[2] = {0};
        to_bytes(this->length, 0, 0, mLen);
        this->bufWrite(mLen[0]);
        this->bufWrite(mLen[1]);

        // send message body
        int count = 0;
        while (count++ < FIFO_LENGTH - 2 && this->xfrd < this->availLen)
        {
            this->bufWrite(this->buf[this->xfrd++]);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        // set packet length for variable length packets
        // this->setProperty(G_PKT, 2, P_PKT_FIELD_2_LENGTH2, mLen);

        // start tx
        // enter rx state after tx
        this->setTXMode();
        this->state = STATE_TX;

        return true;
    }
    return false;
}

uint16_t MockRadio::writeTXBuf(const uint8_t *data, uint16_t len)
{
    // make sure we are in the process of transmitting a message
    // make sure there are some bytes to copy
    // make sure length is not 0 (for some reason)
    // make sure we do not already have enough bytes
    if (this->state == STATE_TX && len > 0 && this->length > 0 && this->availLen < this->length)
    {
        if (this->availLen + len > this->length)
            len = this->length - this->availLen;
        // copy from the array into the internal buf
        memcpy(this->buf + this->availLen, data, len);
        this->availLen += len;
        // len will be the number of bytes copied
        return len;
    }
    // return 0 since no bytes were copied
    return 0;
}

uint16_t MockRadio::readRXBuf(uint8_t *data, uint16_t len)
{
    // make sure we are in the process of receiving a message or a full message is available
    // make sure the user asked for some bytes
    // make sure xfrd is not 0
    // make sure we have not already copied all the available bytes
    if ((this->available && len > 0) || (this->state == STATE_RX && this->availLen > 0 && this->availLen < this->xfrd))
    {
        if (this->state == STATE_RX && this->availLen + len > this->xfrd)
            len = this->xfrd - this->availLen;
        // copy from the internal buf into the array
        memcpy(data, this->buf + this->availLen, len);
        this->availLen += len;
        // len will be the number of bytes copied
        return len;
    }
    // return 0 since no bytes were copied
    return 0;
}

void MockRadio::update()
{
    if (this->state == STATE_TX_COMPLETE)
    {
        if (this->gpio2()) // RX state
            this->state = STATE_RX;

        else if (!this->gpio3()) // ready state (not RX, must go through STATE_ENTER_TX to get to TX)
            this->state = STATE_IDLE;
    }

    if (this->state == STATE_RX_COMPLETE)
    {
        // if (this->gpio2()) // RX state
        //     this->state = STATE_RX;

        if (!this->gpio2() && !this->gpio3()) // ready state (not RX, must go through STATE_ENTER_TX to get to TX)
        {
            this->state = STATE_IDLE;
            // Serial.println("set to idle after rx complete");
        }
    }

    if (this->TXEmptyFlag && !this->gpio0())
    {
        this->TXEmptyFlag = false;
        // Serial.println("update() reset TX took: ");
        // Serial.println(micros() - this->debugTimer);
    }

    if (this->RXFullFlag && !this->gpio1())
    {
        this->RXFullFlag = false;
        // Serial.println("update() reset RX took: ");
        // Serial.println(micros() - this->debugTimer);
    }

    if (this->state == STATE_TX)
    {
        this->handleTX();
    }

    if (this->state == STATE_RX)
    {
        this->handleRX();
    }

    this->internalUpdate();
}

bool MockRadio::avail()
{
    // if we are not in receive mode, enter receive mode
    if (this->state == STATE_IDLE)
        this->rx();
    else
        // otherwise return whether we have fully received a message
        return this->available;
    return false;
}

bool MockRadio::send(Data &data)
{
    // encode the data
    this->m.encode(&data);

    // send the data
    return this->tx(this->m.buf, this->m.size);
}

bool MockRadio::receive(Data &data)
{
    // check if we have received the whole message
    if (this->state == STATE_RX_COMPLETE)
    {
        // decode the message
        this->available = false;
        this->m.fill(this->buf, this->length)->decode(&data);
        return true;
    }
    return false;
}

int MockRadio::RSSI() { return 0; }

bool MockRadio::gpio0()
{
    return this->gpio0State;
}

bool MockRadio::gpio1()
{
    return this->gpio1State;
}

bool MockRadio::gpio2()
{
    return this->gpio2State;
}

bool MockRadio::gpio3()
{
    return this->gpio3State;
}

void MockRadio::bufWrite(uint8_t c)
{
    // push to end of buffer
    this->internalBuf[this->intBufP++] = c;

    this->gpio0State = this->intBufP < MockRadio::TX_THRESH;
    this->gpio1State = this->intBufP > MockRadio::RX_THRESH;
}

uint8_t MockRadio::bufRead()
{
    // read first byte and shift
    uint8_t c = this->internalBuf[0];
    memcpy(this->internalBuf, this->internalBuf + 1, this->intBufP--);

    this->gpio0State = this->intBufP < MockRadio::TX_THRESH;
    this->gpio1State = this->intBufP > MockRadio::RX_THRESH;
    return c;
}

void MockRadio::bufClear()
{
    memset(this->internalBuf, 0, sizeof(this->internalBuf));
    this->intBufP = 0;
}

void MockRadio::setTXMode()
{
    this->totalBytes = 0;
    this->gpio3State = true;
    this->lastUpdate = micros();
}

void MockRadio::setRXMode()
{
    this->totalBytes = 0;
    this->gpio2State = true;
    this->lastUpdate = micros();
}

void MockRadio::internalUpdate()
{
    // RX mode
    if (this->gpio2State && micros() - this->lastUpdate > (8 / this->dataRate) * 1000000)
    {
        if (this->s->available())
        {
            this->lastUpdate = micros();
            // push to end of buffer
            this->internalBuf[this->intBufP++] = this->s->read();

            this->gpio0State = this->intBufP < MockRadio::TX_THRESH;
            this->gpio1State = this->intBufP > MockRadio::RX_THRESH;

            // get length and check if we have the whole message
            // WILL BREAK IF RX_THRESH < 1
            if (this->totalBytes == 0)
            {
                this->internalLength = 0;
                this->internalLength += this->internalBuf[0] << 8;
            }
            if (this->totalBytes == 1)
            {
                this->internalLength += this->internalBuf[1];
            }
            if (this->internalLength > 0 && this->totalBytes - 2 == this->internalLength)
            {
                this->totalBytes = 0;
                this->gpio2State = false;
            }

            this->totalBytes++;
        }
    }

    // TX mode
    if (this->gpio3State && micros() - this->lastUpdate > (8 / this->dataRate) * 1000000)
    {
        this->lastUpdate = micros();

        // get length and check if we have sent the whole message
        if (this->totalBytes == 0)
        {
            this->internalLength = 0;
            this->internalLength += this->internalBuf[0] << 8;
        }
        if (this->totalBytes == 1)
        {
            this->internalLength += this->internalBuf[0];
        }
        if (this->internalLength > 0 && this->totalBytes - 2 == this->internalLength)
        {
            this->totalBytes = 0;
            this->gpio3State = false;
        }

        this->totalBytes++;

        // read first byte and shift
        this->s->write(this->internalBuf[0]);
        memcpy(this->internalBuf, this->internalBuf + 1, this->intBufP--);

        this->gpio0State = this->intBufP < MockRadio::TX_THRESH;
        this->gpio1State = this->intBufP > MockRadio::RX_THRESH;
    }
}

void MockRadio::from_bytes(uint16_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 2 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8); // starts shifted all the way left, then moves right
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            val += arr[pos++] << (i * 8); // starts shifted all the way right, then moves left
        }
    }
}

void MockRadio::from_bytes(uint32_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 4 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            val += arr[pos++] << (i * 8);
        }
    }
}

void MockRadio::from_bytes(uint64_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 8 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            val += arr[pos++] << (i * 8);
        }
    }
}

void MockRadio::to_bytes(uint16_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 2 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            // isolate the leftmost byte, moving one step right each iteration
            // then shift the current byte all the way to the right so it fits in a uint8_t
            arr[pos++] = (val & ((uint16_t)0x00ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            // isolate the rightmost byte, moving one step left each iteration
            // then shift the current byte all the way to the right so it fits in a uint8_t
            arr[pos++] = (val & ((uint16_t)0x00ff << (i * 8))) >> (i * 8);
        }
    }
}

void MockRadio::to_bytes(uint32_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 4 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            arr[pos++] = (val & ((uint32_t)0x000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            arr[pos++] = (val & ((uint32_t)0x000000ff << (i * 8))) >> (i * 8);
        }
    }
}

void MockRadio::to_bytes(uint64_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 8 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            arr[pos++] = (val & ((uint64_t)0x00000000000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            arr[pos++] = (val & ((uint64_t)0x00000000000000ff << (i * 8))) >> (i * 8);
        }
    }
}