#include "Si4463.h"

Si4463::Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig)
{
    // update internal variables with hardware configuration
    this->mod = hConfig.mod;
    this->dataRate = hConfig.dataRate;
    this->freq = hConfig.freq;
    this->pwr = hConfig.pwr;
    this->preambleLen = hConfig.preambleLen;
    this->preambleThresh = hConfig.preambleThresh;

    // update internal variables with pin/SPI configuration
    this->spi = pConfig.spi;
    this->_sdn = pConfig.sdn;
    this->_cs = pConfig.cs;
    this->_irq = pConfig.irq;

    this->_gp0 = pConfig.gpio0;
    this->_gp1 = pConfig.gpio1;
    this->_gp2 = pConfig.gpio2;
    this->_gp3 = pConfig.gpio3;
}

bool Si4463::begin()
{
    // NOTE: Settings are currently to match Si446X library
    // set pins for correct modes
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    pinMode(_sdn, OUTPUT);
    pinMode(_irq, INPUT);
    pinMode(_gp0, INPUT);
    pinMode(_gp1, INPUT);
    pinMode(_gp2, INPUT);
    pinMode(_gp3, INPUT);

    this->spi->begin();

    this->shutdown(true);
    delayMicroseconds(10);
    this->shutdown(false);

    // complete power on sequence
    this->powerOn();

    // clear pending interrupts
    uint8_t cIntArgs[3] = {0, 0, 0};
    uint8_t rIntArgs[8] = {};
    sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);

    //Serial.println("INTERRUPTS");
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(rIntArgs[i], BIN);
    // }

    uint8_t argst[2] = {};
    //Serial.println("DEVICE_STATE");
    sendCommandR(C_REQUEST_DEVICE_STATE, 2, argst);
    //Serial.println(argst[0]);
    //Serial.println(argst[1]);

    // check part info to make sure proper communication has been established
    uint8_t args[8] = {0};
    this->sendCommandR(C_PART_INFO, 8, args);
    //Serial.println("PART_INFO");
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(args[i], HEX);
    // }

    this->sendCommandR(C_PART_INFO, 8, args);

    uint16_t partNo = 0;
    from_bytes(partNo, 1, 0, args);
    if (partNo != PART_NO)
        return false; // Error: did not receive the correct part number

    // set the global config, this is the defaults, but apparently a reserved field needs to be set manually
    this->setProperty(G_GLOBAL, P_GLOBAL_CONFIG, 0b01110000);

    this->setProperty(G_GLOBAL, P_GLOBAL_XO_TUNE, 0x62); // from rf4463f30 datasheet

    // set modem (frequency related) config
    this->setModemConfig(this->mod, this->dataRate, this->freq);
    // set power level (127 = ~20 dBm)
    this->setPower(this->pwr);
    // turn on AFC
    this->setAFC(true);
    // set preamble configuration
    this->setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG, 0b00110001);
    // set preamble length
    this->setProperty(G_PREAMBLE, P_PREAMBLE_TX_LENGTH, this->preambleLen);
    // set preamble threshold
    this->setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG_STD_1, this->preambleThresh & 0b01111111); // first bit must be 0
    // leaving sync word params at defaults

    // // set defaults for gpio pins
    this->setPins(PIN_RX_STATE, PIN_TX_STATE, PIN_RX_STATE, PIN_TX_STATE, PIN_VALID_PREAMBLE, false);

    // // set defaults for FRRs
    this->setFRRs(FRR_CURRENT_STATE, FRR_LATCHED_RSSI, FRR_INT_MODEM_PEND, FRR_INT_PH_STATUS);

    // // packet handling setup, doubly make sure this is being done correctly
    // this->setProperty(G_PKT, P_PKT_LEN, 0b00011010);              // set received field length to be 2 bytes, leave in the length bytes, and set the variable length field to field 2
    // this->setProperty(G_PKT, P_PKT_LEN_FIELD_SOURCE, 0b00000001); // set the length field to be field 1
    // this->setProperty(G_PKT, P_PKT_TX_THRESHOLD, 54);             // fire interrupt when 10 bytes in FIFO
    // this->setProperty(G_PKT, P_PKT_RX_THRESHOLD, 54);             // fire interrupt when 54 bytes in FIFO
    // // set the length of the length field (field 1) to be 4 bytes
    // uint8_t lengthFieldLen[2] = {0, 4};
    // this->setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, lengthFieldLen);
    // // set the length of the data field (field 2) to be the maximum data length of 8191 bytes
    // uint8_t dataMaxLen[2] = {0};
    // to_bytes(this->maxLen, 0, 0, dataMaxLen);
    // this->setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, dataMaxLen);

    this->setRegisters();

    uint8_t cIntArgs2[3] = {0, 0, 0};
    uint8_t rIntArgs2[8] = {};
    //Serial.println("INTERRUPTS");
    sendCommand(C_GET_INT_STATUS, 3, cIntArgs2, 8, rIntArgs2);
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(rIntArgs2[i], BIN);
    // }

    // enter idle state
    uint8_t cIdleArgs[1] = {0b00000011};
    sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

    return true;
}

bool Si4463::tx(const uint8_t *message, int len)
{
    if (len > this->maxLen)
        return false;

    // Store message in buffer
    this->length = len;
    this->xfrd = 0;
    memcpy(this->buf, message, this->length);

    // Start TX state machine
    if (this->state == STATE_IDLE || this->state == STATE_RX || this->state == STATE_RX_COMPLETE)
    {
        // Enter idle state
        uint8_t cIdleArgs[1] = {0b00000011};
        sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

        // Clear FIFO
        uint8_t cClearFIFO[1] = {0b00000011};
        sendCommandC(C_FIFO_INFO, 1, cClearFIFO);

        // Fill initial FIFO chunk
        fillTXFIFO();

        // Set packet length
        uint8_t cLen[2] = {0, this->length & 0x00ff};
        setProperty(G_PKT, 2, P_PKT_FIELD_2_LENGTH2, cLen);

        // Start TX - non-blocking
        uint8_t txArgs[6] = {0, 0b10000000, 0, 0, 0, 0};
        spi_write(C_START_TX, sizeof(txArgs), txArgs);

        this->state = STATE_ENTER_TX;
        return true;
    }
    return false;
}

void Si4463::fillTXFIFO()
{
    digitalWrite(this->_cs, LOW);
    this->spi->transfer(C_WRITE_TX_FIFO);
    
    // Fill FIFO with next chunk
    int count = 0;
    while (count++ < TX_FIFO_SIZE && this->xfrd < this->length)
    {
        this->spi->transfer(this->buf[this->xfrd++]);
    }
    digitalWrite(this->_cs, HIGH);
}

void Si4463::handleTX()
{
    if (this->state == STATE_TX && gpio1()) // FIFO almost empty
    {
        if (longMsgInProgress && longMsgHead < longMsgTail)
        {
            // Fill next chunk from long message buffer
            int remaining = longMsgTail - longMsgHead;
            int chunkSize = min(TX_FIFO_SIZE, remaining);
            
            digitalWrite(this->_cs, LOW);
            this->spi->transfer(C_WRITE_TX_FIFO);
            
            for (int i = 0; i < chunkSize; i++)
            {
                this->spi->transfer(longMsgBuffer[longMsgHead++]);
            }
            
            digitalWrite(this->_cs, HIGH);
        }
        else if (this->xfrd == this->length)
        {
            this->state = STATE_TX_COMPLETE;
            longMsgInProgress = false;
            // Clear buffers
            memset(this->buf, 0, this->length);
            this->length = 0;
            this->xfrd = 0;
        }
    }
}

bool Si4463::rx()
{
    // make sure we aren't already in RX mode
    if (this->state == STATE_IDLE)
    {
        // enter idle state
        uint8_t cIdleArgs[1] = {0b00000011};
        sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

        // clear fifo
        uint8_t cClearFIFO[1] = {0b00000011};
        sendCommandC(C_FIFO_INFO, 1, cClearFIFO);

        // clear first two sets of interrupts
        uint8_t cIntArgs2[3] = {0, 0, 0xff};
        sendCommandR(C_GET_INT_STATUS, 3, cIntArgs2);

        // enter RX mode
        uint8_t rxArgs[7] = {0, 0, 0, 0, 0, 0b00000011, 0b00000001};
        spi_write(C_START_RX, 7, rxArgs);
        this->state = STATE_ENTER_RX;
        return true;
    }
    return false;
}

void Si4463::handleRX()
{
    // we need to be in RX mode to be receiving a message
    if (this->state == STATE_RX)
    {
        uint8_t cIntArgs[3] = {0, 0, 0};
        uint8_t rIntArgs[8] = {};
        sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);

        if (rIntArgs[4] & 0b00000001) // Latched RSSI
        {
            this->rssi = this->readFRR(0);
        }

        if (rIntArgs[2] & 0b00010000) // PACKET_RX_PEND
        {
            digitalWrite(this->_cs, LOW);

            // read from RX FIFO
            this->spi->transfer(C_READ_RX_FIFO);

            // if the internal length and xfrd variables are 0, then this is the first part of the message
            if (this->xfrd == 0)
            {
                // so we need to read length
                // uint8_t mLen[2] = {0};
                // mLen[0] = ;
                // mLen[1] = this->spi->transfer(0x00);
                // convert individual bytes to uint16_t
                // from_bytes(this->length, 0, 0, mLen);
                this->length = this->spi->transfer(0x00);
                // make sure the message is not too long (could be erroneous transmission)
                if (this->length > this->maxLen)
                {
                    this->length = 0;
                    return; // error, message too long
                }
            }

            // recevie message data
            while (this->xfrd < this->length) // MESSAGE MUST BE LESS THAN FIFO LENGTH UNTIL THIS CONDITION CHANGES
            {
                this->buf[this->xfrd++] = this->spi->transfer(0x00);
            }

            digitalWrite(this->_cs, HIGH);

            // if we've transferred length bytes, we've received the whole message
            if (this->xfrd == this->length)
            {
                // automatically placed into an idle state
                this->state = STATE_RX_COMPLETE;
                this->available = true;
                // only reset xfrd
                // length and buf need to stay so they can be read
                this->xfrd = 0;
            }
        }
        if (rIntArgs[2] & 0b00001000)
        {
            Serial.println("Invalid packet! CRC failed.");
        }
    }
}

void Si4463::update()
{
    // save this for later
    // if (this->state == STATE_ENTER_TX && gpio3())
    //     this->state = STATE_TX;
    // if (this->state == STATE_ENTER_RX && gpio2())
    //     this->state = STATE_RX;

    // slightly worse version to use while we don't have access to GPIO 2 and 3 (COTS radio)
    // Serial.println(gpio1());
    // bool cts = checkCTS();
    if (this->state == STATE_ENTER_TX)
    {
        // this->state = STATE_TX;
        // Serial.println("FRR");
        // Serial.println(this->readFRR(0));
        // uint8_t args[2] = {};
        // uint8_t argst[1] = {0};
        // Serial.println("FIFO_INFO");
        // sendCommand(C_FIFO_INFO, 1, argst, 2, args);
        // Serial.println(args[0]);
        // Serial.println(args[1]);
        uint8_t status = this->readFRR(1);
        if (status == 7) // TX state
        {
            this->state = STATE_TX;
        }
        if (status == 8) // RX state
        {
            this->state = STATE_RX;
        }
        if (status == 3) // ready state
        {
            this->state = STATE_IDLE;
        }
    }

    if (this->state == STATE_TX_COMPLETE)
    {
        uint8_t status = this->readFRR(1);
        // Serial.println("TX_COMPLETE");
        if (status == 8) // RX state
        {
            this->state = STATE_RX;
        }
        if (status == 3) // ready state
        {
            this->state = STATE_IDLE;
        }
        else
        {
            // Serial.println(status);
        }
    }

    if (this->state == STATE_ENTER_RX)
    {
        uint8_t status = this->readFRR(1);
        if (status == 8) // RX state
        {
            this->state = STATE_RX;
        }
        if (status == 3) // ready state
        {
            this->state = STATE_IDLE;
        }
    }

    if (this->state == STATE_RX_COMPLETE)
    {
        uint8_t status = this->readFRR(1);
        // Serial.println("RX_COMPLETE");
        if (status == 8) // RX state
        {
            this->state = STATE_RX;
        }
        if (status == 3) // ready state
        {
            this->state = STATE_IDLE;
        }
    }

    // check if we are transmitting and the FIFO is almost empty
    if (this->state == STATE_TX)
    {
        handleTX();
    }
    // check if we are receving and the FIFO is almost full
    if (this->state == STATE_RX) // for now read interrupts here instead of checking GPIO
    {
        handleRX();
    }
}

bool Si4463::send(Data &data)
{
    // encode the data
    this->m.encode(&data);

    uint8_t cIntArgs[3] = {0, 0, 0};
    uint8_t rIntArgs[8] = {};
    //Serial.println("INTERRUPTS");
    sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(rIntArgs[i], BIN);
    // }

    //Serial.println(this->readFRR(0));

    // send the data
    return this->tx(this->m.buf, this->m.size);
}

bool Si4463::receive(Data &data)
{
    // check if we have received the whole message
    if (this->state == STATE_RX_COMPLETE)
    {
        // decode the message
        this->available = false;
        m.fill(this->buf, this->length)->decode(&data);
        return true;
    }
    return false;
}

int Si4463::RSSI() { return this->rssi / 2 - 64 - 70; } // magic formula from datasheet

bool Si4463::avail()
{
    // if we are not in receive mode, enter receive mode
    if (this->state == STATE_IDLE)
        this->rx();
    else
        // otherwise return whether we have fully received a message
        return this->available;
    return false;
}

void Si4463::setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq)
{
    // set modulation
    setProperty(G_MODEM, P_MODEM_MOD_TYPE, mod);
    setProperty(G_MODEM, P_MODEM_MAP_CONTROL, 0x00);

    // set modulation dependant properties
    uint8_t pktConfArgs = 0b00000000;
    uint8_t pktFieldConfArgs = 0b00000000;
    // need to enable 4 FSK at packet handler and field level
    if (mod == MOD_4FSK || mod == MOD_4GFSK)
    {
        pktConfArgs |= 0b01000000;
        pktFieldConfArgs |= 0b00010000;
    }
    // setup all packet fields
    setProperty(G_PKT, P_PKT_CONFIG1, pktConfArgs);
    setProperty(G_PKT, P_PKT_FIELD_1_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_FIELD_2_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_FIELD_3_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_FIELD_4_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_FIELD_5_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_RX_FIELD_1_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_RX_FIELD_2_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_RX_FIELD_3_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_RX_FIELD_4_CONFIG, pktConfArgs);
    setProperty(G_PKT, P_PKT_RX_FIELD_5_CONFIG, pktConfArgs);

    // figure out which band to use

    // parallel arrays to find correct enum
    uint8_t bandConfigs[6] = {BAND_150, BAND_225, BAND_300, BAND_450, BAND_600, BAND_900};
    float bandDivs[6] = {24.0, 16.0, 12.0, 8.0, 6.0, 4.0};
    int bands[6] = {150, 225, 300, 450, 600, 900};

    int freqBand = freq / 1e6; // purposefully truncated due to integer division
    // index of correct band
    int band = -1;
    // fInt and fFrac (see datasheet for formula)
    uint8_t fInt = 0;
    uint32_t fFrac = 0;

    if (!((freqBand >= 142 && freqBand <= 175) ||
          (freqBand >= 284 && freqBand <= 350) ||
          (freqBand >= 350 && freqBand <= 420) ||
          (freqBand >= 420 && freqBand <= 525) ||
          (freqBand >= 850 && freqBand <= 1050)))
    {
        Serial.println("Error: cannot tune to this frequency");
        return; // Error: cannot tune to this frequency
    }

    // loop through each band
    for (int i = 0; i < 5; i++)
    {
        // check if our frequency is above the current band and below the next band
        if (freqBand > bands[i] && freqBand < bands[i + 1])
        {
            // find the difference between our frequency and the current band
            int lowerDiff = freq - bands[i] * 1e6;
            // find the difference between our frequency and the next band
            int upperDiff = -1 * (freq - bands[i + 1] * 1e6);
            // see which difference is smaller
            if (lowerDiff < upperDiff)
                band = i;
            else
                band = i + 1;

            // see API reference FREQ_CONTROL_INTE for math
            int combinedIntFrac = freq / (2 * (int)30e6 / (int)bandDivs[band]);               // we want to truncate to find the integer part
            fInt = combinedIntFrac - 1;                                                       // subtract one since fraction part is between 1-2
            float intFracFraction = freq / (2 * 30e6 / bandDivs[band]) - combinedIntFrac + 1; // don't want to truncate to get fractional part
            fFrac = intFracFraction * pow(2, 19);                                             // from datasheet math, may truncate, but this is as close as we can get
            break;
        }
    }

    // set frequency
    // make sure we found the band and computed fInt and fFrac
    if (band != -1 && !(fInt == 0 && fFrac == 0))
    {
        setProperty(G_MODEM, P_MODEM_CLKGEN_BAND, bandConfigs[band]);
        uint8_t freqArgs[4] = {(uint8_t)(fInt & 0b01111111)};          // first bit must be 0
        fFrac &= 0x00FFFFFF;                                           // first byte must be 0
        to_bytes(fFrac, 1, 1, freqArgs);                               // put fFrac into freqArgs
        setProperty(G_FREQ_CONTROL, 4, P_FREQ_CONTROL_INTE, freqArgs); // set FREQ_CONTROL_INTE and FREQ_CONTROL_FRAC
    }
    else
    {
        Serial.println("Error: could not find frequency");
        return; // Error: could not find frequency
    }

    // set data rate
    uint8_t drArgs[7] = {};
    to_bytes(dataRate, 0, 1, drArgs);
    setProperty(G_MODEM, 7, P_MODEM_DATA_RATE3, drArgs);

    // set frequency deviation
    // see datasheet for math, this includes everything but a bitrate dependent factor and 4-FSK correction
    float fDev = (double)pow(2, 19) * (double)bandDivs[band] / (float)(2 * 30e6);
    if (mod == MOD_4FSK || mod == MOD_4GFSK)
        fDev /= 3.0; // we want the inner deviation when we are using 4-FSK
    switch (dataRate)
    {
    case DR_500b:
        fDev *= (500.0 / 2.0); // this adds the desired frequency to fdev, should be dataRate / 2
        break;
    case DR_4_8k:
        fDev *= (4800.0 / 2.0);
        break;
    case DR_9_6k:
        fDev *= (9600.0 / 2.0);
        break;
    case DR_40k:
        fDev *= (40e3 / 2.0);
        break;
    case DR_100k:
        fDev *= (100e3 / 2.0);
        break;
    case DR_120k:
        fDev *= (120e3 / 2.0);
        break;
    case DR_500k:
        fDev *= (500e3 / 2.0);
        break;
    default:
        return; // Error: data rate is not part of the Si4463DataRate enum (should never happen)
    }

    uint8_t fDevArgs[3] = {};

    // first 7 bits should be 0
    to_bytes((uint32_t)fDev & 0x0001FFFF, 0, 1, fDevArgs);
    setProperty(G_MODEM, 3, P_MODEM_FREQ_DEV3, fDevArgs);

    // set additional config
    // uint8_t MODEM_TX_RAMP_DELAY[12] = {0x01, 0x00, 0x08, 0x03, 0x80, 0x00, 0xB0, 0x10, 0x0C, 0xE8, 0x00, 0x4E};
    // setProperty(G_MODEM, 12, P_MODEM_TX_RAMP_DELAY, MODEM_TX_RAMP_DELAY);
    // uint8_t MODEM_BCR_NCO_OFFSET_2[12] = {0x06, 0x8D, 0xB9, 0x00, 0x00, 0x02, 0xC0, 0x08, 0x00, 0x12, 0x00, 0x23};
    // setProperty(G_MODEM, 12, P_MODEM_BCR_NCO_OFFSET3, MODEM_BCR_NCO_OFFSET_2);
    // uint8_t MODEM_AFC_LIMITER[3] = {0x01, 0x5C, 0xA0};
    // setProperty(G_MODEM, 3, P_MODEM_AFC_LIMITER2, MODEM_AFC_LIMITER);
    // uint8_t MODEM_AGC_CONTROL = 0xE0;
    // setProperty(G_MODEM, P_MODEM_AGC_CONTROL, MODEM_AGC_CONTROL);
    // uint8_t MODEM_AGC_WINDOW_SIZE[12] = {0x11, 0x11, 0x11, 0x80, 0x1A, 0x20, 0x00, 0x00, 0x28, 0x0C, 0xA4, 0x23};
    // setProperty(G_MODEM, 12, P_MODEM_AGC_WINDOW_SIZE, MODEM_AGC_WINDOW_SIZE);
    // uint8_t MODEM_RAW_CONTROL[5] = {0x03, 0x00, 0x85, 0x01, 0x00};
    // setProperty(G_MODEM, 5, P_MODEM_RAW_CONTROL, MODEM_RAW_CONTROL);
    // uint8_t MODEM_RSSI_JUMP_THRESH[4] = {0x06, 0x09, 0x10, 0x40};
    // setProperty(G_MODEM, 4, P_MODEM_RSSI_JUMP_THRESH, MODEM_RSSI_JUMP_THRESH);
    // uint8_t MODEM_RAW_SEARCH2[2] = {0x94, 0x0A};
    // setProperty(G_MODEM, 2, P_MODEM_RAW_SEARCH2, MODEM_RAW_SEARCH2);
    // uint8_t MODEM_SPIKE_DET[2] = {0x03, 0x07};
    // setProperty(G_MODEM, 2, P_MODEM_SPIKE_DET, MODEM_SPIKE_DET);
    // skip MODEM_RSSI_MUTE
    // uint8_t MODEM_DSA_CTRL1[5] = {0x40, 0x04, 0x04, 0x78, 0x20};
    // setProperty(G_MODEM, 5, P_MODEM_DSA_CTRL_1, MODEM_DSA_CTRL1);
}

void Si4463::setPower(uint8_t pwr)
{
    // const uint8_t PA_MODE = 0b000001000; // this is the default
    // setProperty(G_PA, P_PA_MODE, PA_MODE);
    setProperty(G_PA, P_PA_PWR_LVL, pwr & 0b01111111); // first bit should be 0
}

void Si4463::setPins(Si4463Pin gpio0Mode, Si4463Pin gpio1Mode, Si4463Pin gpio2Mode, Si4463Pin gpio3Mode, Si4463Pin irqMode, bool pullup)
{
    uint8_t pullupMask = 0b00000000;
    // need to set this bit to 1 if we want to pull up the output pins
    if (pullup)
    {
        pullupMask = 0b01000000;
    }
    // put all the mode arguments into an array, except irq which is set to DO_NOTHING
    uint8_t gpioArgs[7] = {(uint8_t)(gpio0Mode | pullupMask), (uint8_t)(gpio1Mode | pullupMask),
                           (uint8_t)(gpio2Mode | pullupMask), (uint8_t)(gpio3Mode | pullupMask),
                           (uint8_t)(PIN_DO_NOTHING | pullupMask), (uint8_t)(PIN_SDO | pullupMask), 0x03};

    // irq can only be set to specific values, if we were given a valid value, update irq in the array here
    if (irqMode < 0x04 || irqMode == 0x07 || irqMode == 0x08 || irqMode == 0x0B || irqMode == 0x0C ||
        (irqMode >= 0x0F && irqMode <= 0x1B) || irqMode == 0x1D || irqMode == 0x1F || irqMode == 0x27)
    {
        gpioArgs[4] = irqMode | pullupMask;
    }

    uint8_t resArgs[7] = {};
    sendCommand(C_GPIO_PIN_CFG, 7, gpioArgs, 7, resArgs);
    //Serial.println("PINS");
    // for (int i = 0; i < 7; i++)
    // {
    //     Serial.println(resArgs[i], HEX);
    // }
}

void Si4463::setFRRs(Si4463FRR regAMode, Si4463FRR regBMode, Si4463FRR regCMode, Si4463FRR regDMode)
{
    // update the behavior of each register individually
    if (regAMode != FRR_NO_CHANGE)
        setProperty(G_FRR_CTL, P_FRR_CTL_A_MODE, regAMode);
    if (regBMode != FRR_NO_CHANGE)
        setProperty(G_FRR_CTL, P_FRR_CTL_B_MODE, regBMode);
    if (regCMode != FRR_NO_CHANGE)
        setProperty(G_FRR_CTL, P_FRR_CTL_C_MODE, regCMode);
    if (regDMode != FRR_NO_CHANGE)
        setProperty(G_FRR_CTL, P_FRR_CTL_D_MODE, regDMode);
}

void Si4463::setAFC(bool enabled)
{
    // AFC gain settings for optimal Doppler tracking
    uint8_t afcGainArgs[2] = {0b10000111, 0x89}; // Increased gain for better tracking
    
    if (!enabled) {
        afcGainArgs[0] = 0b00000011;
        afcGainArgs[1] = 0x69;
    }
    setProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, afcGainArgs);
    
    if (enabled) {
        // Set AFC limits to handle expected Doppler range
        // Max deviation increased to ±3 kHz for rocket applications
        uint8_t afcLimiterArgs[2] = {0b01000000, 0xF0}; 
        setProperty(G_MODEM, 2, P_MODEM_AFC_LIMITER2, afcLimiterArgs);
        
        // Enable AFC feedback and fast tracking mode
        uint8_t afcMiscArgs = 0b11101100; // Added fast tracking bit
        setProperty(G_MODEM, P_MODEM_AFC_MISC, afcMiscArgs);
        
        // Set AFC sampling timing
        uint8_t afcTimingArgs[2] = {0x02, 0x08}; // Sample more frequently
        setProperty(G_MODEM, 2, P_MODEM_AFC_TIMING, afcTimingArgs);
    }
}

bool Si4463::gpio0()
{
    return digitalRead(this->_gp0);
}

bool Si4463::gpio1()
{
    return digitalRead(this->_gp1);
}

bool Si4463::gpio2()
{
    return digitalRead(this->_gp2);
}

bool Si4463::gpio3()
{
    return digitalRead(this->_gp3);
}

bool Si4463::irq()
{
    return digitalRead(this->_irq);
}

void Si4463::shutdown(bool shutdown)
{
    if (shutdown)
    {
        digitalWrite(this->_sdn, HIGH);
    }
    else
    {
        digitalWrite(this->_sdn, LOW);
        while (!gpio1())
        {
            yield();
        }
        spi_write(C_NOP, 0, {});
        while (!gpio1())
        {
            yield();
        }
    }
}

int Si4463::readFRR(int index)
{
    // call readFRRs, but only return the value of the first index in the array
    uint8_t data[4] = {0};
    readFRRs(data, index);
    return data[0];
}

void Si4463::setRegisters()
{
    // 1. MODEM Configuration - Should be modified based on data rate and modulation
    uint8_t MODEM_5_0[] = {0x11, 0x20, 0x0A, 0x03, 0x1E, 0x84, 0x80, 0x09, 0xC9, 0xC3, 0x80, 0x00, 0x0D, 0xA7};
    uint8_t MODEM_5_1[] = {0x11, 0x20, 0x0B, 0x1E, 0x10, 0x20, 0x00, 0xE8, 0x00, 0x4B, 0x06, 0xD3, 0xA0, 0x06, 0xD4};
    uint8_t MODEM_5_2[] = {0x11, 0x20, 0x09, 0x2A, 0x00, 0x00, 0x00, 0x23, 0xC6, 0xD4, 0x00, 0xA9, 0xE0};

    // 2. Frequency Control - Should be modified based on center frequency
    uint8_t FREQ_CONTROL_5_0[] = {0x11, 0x40, 0x04, 0x00, 0x38, 0x0D, 0xDD, 0xDD};

    // 3. Channel Filter - Should be modified based on data rate and frequency deviation
    uint8_t MODEM_CHFLT_5_0[] = {0x11, 0x21, 0x0C, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08};
    uint8_t MODEM_CHFLT_5_1[] = {0x11, 0x21, 0x0C, 0x0C, 0x07, 0x03, 0x15, 0xFC, 0x0F, 0x00, 0xA2, 0x81, 0x26, 0xAF, 0x3F, 0xEE};
    uint8_t MODEM_CHFLT_5_2[] = {0x11, 0x21, 0x0B, 0x18, 0xC8, 0xC7, 0xDB, 0xF2, 0x02, 0x08, 0x07, 0x03, 0x15, 0xFC, 0x0F};

    // 4. Synthesizer - Should be modified based on frequency band
    uint8_t SYNTH_5_0[] = {0x11, 0x23, 0x06, 0x00, 0x34, 0x04, 0x0B, 0x04, 0x07, 0x70};

    // These registers should be calculated and set after the main configuration:
    if (this->dataRate != 0) {
        // Calculate and update MODEM registers based on data rate
        updateModemRegisters(MODEM_5_0, MODEM_5_1, MODEM_5_2);
    }

    if (this->freq != 0) {
        // Calculate and update frequency control registers
        updateFrequencyRegisters(FREQ_CONTROL_5_0);
        // Update synthesizer settings for the frequency band
        updateSynthRegisters(SYNTH_5_0);
    }

    // Update channel filter based on both data rate and frequency deviation
    if (this->dataRate != 0 && this->freq != 0) {
        updateChannelFilterRegisters(MODEM_CHFLT_5_0, MODEM_CHFLT_5_1, MODEM_CHFLT_5_2);
    }

    // Apply the register settings
    setProperty(MODEM_5_0, sizeof(MODEM_5_0));
    setProperty(MODEM_5_1, sizeof(MODEM_5_1));
    setProperty(MODEM_5_2, sizeof(MODEM_5_2));
    setProperty(FREQ_CONTROL_5_0, sizeof(FREQ_CONTROL_5_0));
    setProperty(MODEM_CHFLT_5_0, sizeof(MODEM_CHFLT_5_0));
    setProperty(MODEM_CHFLT_5_1, sizeof(MODEM_CHFLT_5_1));
    setProperty(MODEM_CHFLT_5_2, sizeof(MODEM_CHFLT_5_2));
    setProperty(SYNTH_5_0, sizeof(SYNTH_5_0));
}

// Helper functions to calculate register values:

void Si4463::updateModemRegisters(uint8_t *modem0, uint8_t *modem1, uint8_t *modem2) {
    // Calculate register values based on data rate
    // Update the arrays in place
    // Example calculation for data rate dependent fields
    float symbolRate = this->dataRate;
    // Update NCO speed
    // Update clock recovery settings
    // Update modulation settings
}

void Si4463::updateFrequencyRegisters(uint8_t *freqControl) {
    // Calculate frequency control register values
    // Update the array in place
    // Example: Calculate integer and fractional parts of frequency synthesizer
    uint32_t centerFreq = this->freq;
    // Update frequency control words
}

void Si4463::updateChannelFilterRegisters(uint8_t *chflt0, uint8_t *chflt1, uint8_t *chflt2) {
    // Calculate channel filter coefficients based on data rate and deviation
    // Update the arrays in place
    // Example: Calculate filter bandwidth and coefficients
    float filterBW = this->dataRate * 1.5; // Typical filter bandwidth
    // Update filter coefficients
}

void Si4463::updateSynthRegisters(uint8_t *synth) {
    // Update synthesizer settings based on frequency band
    // Update the array in place
    // Example: Set charge pump current and loop filter values
    uint32_t freqBand = this->freq / 1000000; // MHz
    // Update synthesizer settings for the band
}

void Si4463::setProperty(Si4463Group group, Si4463Property start, uint8_t data)
{
    // three args plus length of the data
    uint8_t cmdArgs[3 + 1] = {group, 1, start, data};
    sendCommandC(C_SET_PROPERTY, 3 + 1, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, Si4463Property start, uint8_t &data)
{
    // three command args, reading 1 byte of data
    uint8_t cmdArgs[3] = {group, 1, start};
    sendCommand(C_GET_PROPERTY, 3, cmdArgs, 1, &data);
}

void Si4463::setProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data)
{
    // make sure we're not exceed the max number of properties the chip can set at a time
    if (num > MAX_NUM_PROPS)
        return;
    // three args plus length of data
    uint8_t cmdArgs[3 + num] = {group, num, start};
    // add data to cmdArgs
    for (int i = 0; i < num; i++)
        cmdArgs[3 + i] = data[i];

    sendCommandC(C_SET_PROPERTY, 3 + num, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data)
{
    // make sure we're not exceed the max number of properties we can get at a time
    if (num > MAX_NUM_PROPS)
        return;
    // three args, reading num bytes of data
    uint8_t cmdArgs[3] = {group, num, start};
    sendCommand(C_GET_PROPERTY, 3, cmdArgs, num, data);
}

void Si4463::setProperty(uint8_t *data, uint8_t size)
{
    digitalWrite(this->_cs, LOW);

    for (int i = 0; i < size; i++)
    {
        char str[5] = {};
        snprintf(str, 5, "%#02x", data[i]);
        Serial.print(str);
        Serial.print(" ");
        this->spi->transfer(data[i]);
    }
    Serial.println();

    digitalWrite(this->_cs, HIGH);

    waitCTS();
}

void Si4463::readFRRs(uint8_t data[4], uint8_t start)
{
    // CS needs to stay low throughout reading FRRs
    digitalWrite(this->_cs, LOW);

    // figure out which FRR to read from first
    uint8_t cmd = C_FRR_A_READ;
    if (start == 1)
        cmd = C_FRR_B_READ;
    if (start == 2)
        cmd = C_FRR_C_READ;
    if (start == 3)
        cmd = C_FRR_D_READ;

    this->spi->transfer(cmd);

    // no need to wait for CTS
    for (int i = 0; i < 4; i++)
        data[i] = this->spi->transfer(0x00);

    digitalWrite(this->_cs, HIGH);
}

void Si4463::powerOn()
{
    // Serial.println("Starting power up");
    // Serial.flush();
    // must wait for CTS before sending power up command
    // waitCTS();

    // Serial.println("CTS high, chip active");
    // Serial.flush();

    uint8_t BOOT_OPTIONS = 0b00000001;
    uint8_t XTAL_OPTIONS = 0b00000000; // assume external crystal (need to change if we have no external crystal)
    uint32_t XO_FREQ = 0x01C9C380;     // 30000000 (30 MHz)

    // assmble power up options
    uint8_t options[6] = {
        BOOT_OPTIONS,
        XTAL_OPTIONS,
    };
    to_bytes(XO_FREQ, 2, 0, options);

    sendCommandC(C_POWER_UP, 6, options);

    while (!gpio1())
    {
        Serial.println(gpio1());
        // delay(1);
    }
}

void Si4463::sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with its args
    spi_write(cmd, argcCmd, argvCmd);
    // read command response
    spi_read(argcRes, argvRes);
}

void Si4463::sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with no args
    spi_write(cmd, 0, {});
    // read command response
    spi_read(argcRes, argvRes);
}

void Si4463::sendCommandC(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd)
{
    // send the cmd with its args
    spi_write(cmd, argcCmd, argvCmd);
    // need to wait for cts in case other commands are called after
    waitCTS();
}

void Si4463::waitCTS()
{
    // blocking while loop (should yield to other functions)
    while (!checkCTS())
    {
        delayMicroseconds(10);
        yield();
    }
}

bool Si4463::checkCTS()
{
    // use READ_CMD_BUFF command to check the value of CTS
    // CS low through entire SPI command
    digitalWrite(this->_cs, LOW);

    // send the command
    this->spi->transfer(C_READ_CMD_BUFF);
    uint8_t cts = this->spi->transfer(0x00);

    digitalWrite(this->_cs, HIGH);
    return cts == 0xff;
}

// private methods

void Si4463::spi_write(uint8_t cmd, uint8_t argc, uint8_t *argv)
{
    // CS low through entire SPI command
    digitalWrite(this->_cs, LOW);

    // send the command
    this->spi->transfer(cmd);
    // send the args
    for (int i = 0; i < argc; i++)
    {
        this->spi->transfer(argv[i]);
    }

    digitalWrite(this->_cs, HIGH);
}

void Si4463::spi_read(uint8_t argc, uint8_t *argv)
{
    // CS low through entire SPI command
    uint8_t pos = 0;

    uint8_t cts = 0x00;
    while (cts != 0xFF)
    {
        digitalWrite(this->_cs, LOW);
        this->spi->transfer(C_READ_CMD_BUFF);
        cts = this->spi->transfer(0x00);
        if (cts != 0xFF)
        {
            digitalWrite(this->_cs, HIGH);
            delayMicroseconds(1);
        }
    }

    // read in the args (CTS must already have been received)
    while (pos < argc)
    {
        argv[pos++] = this->spi->transfer(0x00);
    }

    digitalWrite(this->_cs, HIGH);
}

void Si4463::from_bytes(uint16_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
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

void Si4463::from_bytes(uint32_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
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

void Si4463::from_bytes(uint64_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
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

void Si4463::to_bytes(uint16_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 2 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            // isolate the leftmost byte, moving one step right each iteration
            // then shift the current byte all the way to the right so it fits in a uint8_t
            arr[pos++] = (val & (0x00ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            // isolate the rightmost byte, moving one step left each iteration
            // then shift the current byte all the way to the right so it fits in a uint8_t
            arr[pos++] = (val & (0x00ff << (i * 8))) >> (i * 8);
        }
    }
}

void Si4463::to_bytes(uint32_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 4 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            arr[pos++] = (val & (0x000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            arr[pos++] = (val & (0x000000ff << (i * 8))) >> (i * 8);
        }
    }
}

void Si4463::to_bytes(uint64_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
{
    int iterNum = 8 - bytePos;
    if (MSB)
    {
        for (int i = iterNum - 1; i >= 0; i--)
        {
            arr[pos++] = (val & (0x00000000000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
            arr[pos++] = (val & (0x00000000000000ff << (i * 8))) >> (i * 8);
        }
    }
}

// Basic power function
// exponent must be > 0
// Returns: base^exponent
int pow(int base, int exponent)
{
    // take base^exponent
    int val = base;
    // since we put 1 base in val, that's already base^1
    // so take one off of exponent
    exponent--;
    // multiple the rest of the way
    while (exponent-- > 0)
        val *= base;
    return val;
}

bool Si4463::sendLongMessage(const uint8_t *message, int len)
{
    if (len > MAX_LONG_MSG_SIZE)
        return false;
        
    // Store in long message buffer
    memcpy(longMsgBuffer, message, len);
    longMsgHead = 0;
    longMsgTail = len;
    longMsgInProgress = true;
    
    // Start transmission of first chunk
    return tx(longMsgBuffer, min(TX_FIFO_SIZE, len));
}