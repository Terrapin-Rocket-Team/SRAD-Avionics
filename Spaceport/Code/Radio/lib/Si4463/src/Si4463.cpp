#include "Si4463.h"

Si4463::Si4463()
{
    // update internal variables with hardware configuration
    this->mod = MOD_2GFSK;
    this->dataRate = DR_100k;
    this->freq = 433e6;
    this->pwr = 127;
    this->preambleLen = 48;
    this->preambleThresh = 16;

    // update internal variables with pin/SPI configuration
    this->spi = &SPI;
    this->_sdn = 38;
    this->_cs = 10;
    this->_irq = 33;

    this->_gp0 = 34;
    this->_gp1 = 35;
    this->_gp2 = 36;
    this->_gp3 = 37;
}

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

Si4463::~Si4463()
{
    if (this->WDS_CONFIG != nullptr)
        delete[] this->WDS_CONFIG;
}

bool Si4463::begin()
{
    // set pins for correct modes
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    pinMode(_sdn, OUTPUT);
    pinMode(_irq, INPUT);
    pinMode(_gp0, INPUT);
    pinMode(_gp1, INPUT);
    pinMode(_gp2, INPUT);
    pinMode(_gp3, INPUT);
    this->_cts = this->_irq;

    this->spi->begin();

    if (!this->shutdown(true))
        return false;
    delayMicroseconds(10);
    if (!this->shutdown(false))
        return false;

    // complete power on sequence
    this->powerOn();

    // clear pending interrupts
    // uint8_t cIntArgs[3] = {0, 0, 0};
    // uint8_t rIntArgs[8] = {};
    // sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);
    // Serial.println("INTERRUPTS");
    //  for (int i = 0; i < 8; i++)
    //  {
    //      Serial.println(rIntArgs[i], BIN);
    //  }

    // check part info to make sure proper communication has been established
    uint8_t args[8] = {0};
    this->sendCommandR(C_PART_INFO, 8, args);
    // Serial.println("PART_INFO");
    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(args[i], HEX);
    // }

    uint16_t partNo = 0;
    from_bytes(partNo, 1, 0, args);
    if (partNo != PART_NO)
        return false; // ERROR: did not receive the correct part number

#ifndef RF4463F30
    // set the global config, this is the defaults, but apparently a reserved field needs to be set manually
    this->setProperty(G_GLOBAL, P_GLOBAL_CONFIG, 0b01010000);

    // set clock config
    this->setProperty(G_GLOBAL, P_GLOBAL_XO_TUNE, 0x00);
    this->setProperty(G_GLOBAL, P_GLOBAL_CLK_CFG, 0x00);
#else
    // rf4463 settings
    this->setProperty(G_GLOBAL, P_GLOBAL_CONFIG, 0b01110000);
    this->setProperty(G_GLOBAL, P_GLOBAL_XO_TUNE, 0x62); // from rf4463f30 datasheet
#endif

    // disable interrupts
    this->setProperty(G_INT_CTL, P_INT_CTL_ENABLE, 0x00);

    // set TX and RX thresholds
    this->setTXThreshold(TX_THRESH);
    this->setRXThreshold(RX_THRESH);

    // Set properties from WDS first
    this->applyRadioConfig();
    // set modem (frequency related) config
    this->setModemConfig(this->mod, this->dataRate, this->freq);
    // set power level (127 = ~20 dBm)
    this->setPower(this->pwr);
    // turn on AFC
    this->setAFC(true);

    // set defaults for gpio pins
    this->setPins(PIN_TX_FIFO_EMPTY, PIN_RX_FIFO_FULL, PIN_RX_STATE, PIN_TX_STATE, PIN_CTS, false);
    this->useSPICTS = false;

    // set defaults for FRRs
    this->setFRRs(FRR_CURRENT_STATE, FRR_LATCHED_RSSI, FRR_INT_MODEM_PEND, FRR_INT_PH_STATUS);

    this->setPacketConfig(this->mod, this->preambleLen, this->preambleThresh);

    // TODO: needs update
    // this->performIRCAL();

    // enter idle state
    uint8_t cIdleArgs[1] = {0b00000011};
    this->sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

    return true;
}

bool Si4463::begin(const uint8_t *config, uint32_t length)
{
    // set the config before calling normal being
    this->setRadioConfig(config, length);
    // call and return normal begin
    return this->begin();
}

bool Si4463::tx(const uint8_t *message, int len)
{
    // make sure the packet isn't too long
    if (len > Si4463::MAX_LEN)
        return false; // Error: the packet is too long

    // otherwise add the message to the internal buffer
    this->length = len;
    this->xfrd = 0;
    memcpy(this->buf, message, this->length);
    // Serial.println(this->state);
    //  prefill fifo in idle state
    if (this->state == STATE_IDLE || this->state == STATE_RX || this->state == STATE_RX_COMPLETE)
    {
        // Serial.println("tx");
        //  enter idle state
        uint8_t cIdleArgs[1] = {0b00000011};
        this->sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

        // clear fifo
        uint8_t cClearFIFO[1] = {0b00000011};
        this->sendCommandC(C_FIFO_INFO, 1, cClearFIFO);

        // clear first two sets of interrupts
        // uint8_t cIntArgs2[3] = {0, 0, 0xff};
        // sendCommandC(C_GET_INT_STATUS, 3, cIntArgs2);

        // start spi
        digitalWrite(this->_cs, LOW);

        // write to TX FIFO
        this->spi->transfer(C_WRITE_TX_FIFO);

        // send length
        uint8_t mLen[2] = {0};
        to_bytes(this->length, 0, 0, mLen);
        this->spi->transfer(mLen[0]);
        this->spi->transfer(mLen[1]);

        // send message body
        int count = 0;
        while (count++ < FIFO_LENGTH - 2 && this->xfrd < this->length)
        {
            this->spi->transfer(this->buf[this->xfrd++]);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        digitalWrite(this->_cs, HIGH);

        // set packet length for variable length packets
        this->setProperty(G_PKT, 2, P_PKT_FIELD_2_LENGTH2, mLen);

        // start tx
        // enter rx state after tx
        uint8_t txArgs[6] = {this->channel, 0b10000000, 0, 0, 0, 0};
        this->spi_write(C_START_TX, sizeof(txArgs), txArgs);

        this->state = STATE_TX;

        return true;
    }
    return false;
}

void Si4463::handleTX()
{
    // Serial.println("handleTX1");
    // Serial.println(this->xfrd);
    // Serial.println(this->length);
    // this function assumes we are in tx mode already, so check that we are in tx mode
    if (this->gpio0() && this->xfrd < this->length)
    {
        // Serial.println("handleTX");
        digitalWrite(this->_cs, LOW);

        // write to the TX FIFO
        this->spi->transfer(C_WRITE_TX_FIFO);

        // write remaining data
        int count = 0;
        while (count++ < TX_THRESH && this->xfrd < this->length)
        {
            this->spi->transfer(this->buf[this->xfrd++]);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        digitalWrite(this->_cs, HIGH);
    }
    // if we've sent this->length bytes, the message is complete
    // needed
    if (this->xfrd == this->length)
    {
        // automatically placed into an idle state
        this->state = STATE_TX_COMPLETE;
        // clear internal variables
        memset(this->buf, 0, this->length);
        this->length = 0;
        this->xfrd = 0;
    }
}

bool Si4463::rx()
{
    // make sure we aren't already in RX mode
    if (this->state == STATE_IDLE)
    {
        // enter idle state
        uint8_t cIdleArgs[1] = {0b00000011};
        this->sendCommandC(C_CHANGE_STATE, 1, cIdleArgs);

        // clear fifo
        uint8_t cClearFIFO[1] = {0b00000011};
        this->sendCommandC(C_FIFO_INFO, 1, cClearFIFO);

        // clear first two sets of interrupts
        // uint8_t cIntArgs2[3] = {0, 0, 0xff};
        // this->sendCommandC(C_GET_INT_STATUS, 3, cIntArgs2);

        // set back to max length for rx mode?
        uint8_t cLen2[2] = {0x1f, 0xff};
        this->setProperty(G_PKT, 2, P_PKT_FIELD_2_LENGTH2, cLen2);

        // enter RX mode
        uint8_t rxArgs[7] = {this->channel, 0, 0, 0, 0, 0b00000011, 0b00000001};
        this->spi_write(C_START_RX, 7, rxArgs);
        this->state = STATE_RX;
        return true;
    }
    return false;
}

void Si4463::handleRX()
{
    // assume we are in RX mode
    // this is how we read the packet until we have less than the RX FIFO THRESH left
    if (this->gpio1()) // valid preamble and more than RX_THRESH bytes in FIFO
    {
        // Serial.println("here");
        // Serial.println(this->xfrd);
        // Serial.println(this->length);
        // uint8_t cClearFIFO[1] = {0b00000000};
        // uint8_t rClearFIFO[2] = {0x00, 0x00};
        // sendCommand(C_FIFO_INFO, 1, cClearFIFO, 2, rClearFIFO);
        // Serial.println("FIFO STATUS");
        // for (int i = 0; i < sizeof(rClearFIFO); i++)
        //     Serial.println(rClearFIFO[i]);
        // this->hasPacket = true;
        // rssi should be available
        this->rssi = this->readFRR(1);
        digitalWrite(this->_cs, LOW);

        // read from RX FIFO
        this->spi->transfer(C_READ_RX_FIFO);

        // holds data received this iteration
        int count = 0;

        // if the internal length and xfrd variables are 0, then this is the first part of the message
        if (this->xfrd == 0)
        {
            // so we need to read length
            uint8_t mLen[2] = {0x00, 0x00};
            mLen[0] = this->spi->transfer(0x00);
            mLen[1] = this->spi->transfer(0x00);
            // convert individual bytes to uint16_t
            from_bytes(this->length, 0, 0, mLen);
            // Serial.print("len ");
            // Serial.println(this->length);
            count += 2;
            // make sure the message is not too long (could be erroneous transmission)
            if (this->length > Si4463::MAX_LEN || this->length == 0)
            {
                this->length = 0;
                return; // error, message too long or too short
            }
        }

        // receive message data
        while (this->xfrd < this->length && count < RX_THRESH)
        {
            count++;
            this->buf[this->xfrd++] = this->spi->transfer(0x00);
            // Serial.print((char)this->buf[this->xfrd - 1]);
        }
        // Serial.println();

        digitalWrite(this->_cs, HIGH);

        // if we've transferred length bytes, we've received the whole message
        if (this->xfrd == this->length && this->length > 0)
        {
            // automatically placed into an idle state
            this->state = STATE_RX_COMPLETE;
            this->available = true;
            // this->hasPacket = false;
            // only reset xfrd
            // length and buf need to stay so they can be read
            this->xfrd = 0;
        }
    }
    if (this->length > 0 && (this->length - this->xfrd < RX_THRESH))
    {
        // Serial.println("Here2");
        // Serial.println(this->xfrd);
        // Serial.println(this->length);
        uint8_t cFIFOInfo[1] = {0b00000000};
        uint8_t rFIFOInfo[2] = {0x00, 0x00};
        sendCommand(C_FIFO_INFO, 1, cFIFOInfo, 2, rFIFOInfo);
        // Serial.println("FIFO STATUS");
        // for (int i = 0; i < sizeof(rFIFOInfo); i++)
        //     Serial.println(rFIFOInfo[i]);

        // dont need to send an SPI command unless there's actually bytes to read
        if (rFIFOInfo[0] > 0) // TODO: is there a better way to do this?
        {

            digitalWrite(this->_cs, LOW);

            // read from RX FIFO
            this->spi->transfer(C_READ_RX_FIFO);

            // receive message data
            int bytes = 0;
            while (this->xfrd < this->length && bytes < rFIFOInfo[0])
            {
                bytes++;
                this->buf[this->xfrd++] = this->spi->transfer(0x00);
                // Serial.print((char)this->buf[this->xfrd - 1]);
            }
            // Serial.println();

            digitalWrite(this->_cs, HIGH);
        }

        // if we've transferred length bytes, we've received the whole message
        if (this->xfrd == this->length)
        {
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

void Si4463::update()
{
#ifndef RF4463F30

    if (this->state == STATE_TX_COMPLETE)
    {
        if (this->gpio2()) // RX state
            this->state = STATE_RX;

        else // ready state (not RX, must go through STATE_ENTER_TX to get to TX)
            this->state = STATE_IDLE;
    }

    if (this->state == STATE_RX_COMPLETE)
    {
        if (this->gpio2()) // RX state
            this->state = STATE_RX;

        else // ready state (not RX, must go through STATE_ENTER_TX to get to TX)
            this->state = STATE_IDLE;
    }
#else
    // slightly worse version to use for rf4463

    if (this->state == STATE_TX_COMPLETE)
    {
        uint8_t status = this->readFRR(0);
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
        uint8_t status = this->readFRR(0);
        if (status == 8) // RX state
        {
            this->state = STATE_RX;
        }
        if (status == 3) // ready state
        {
            this->state = STATE_IDLE;
        }
    }
#endif

    // throttle reading and writing a bit cause the teensy does it faster than the FIFO status pins update
    if (millis() - this->timer > this->byteDelay)
    {
        this->timer = millis();
        // check if we are transmitting and the FIFO is almost empty
        if (this->state == STATE_TX)
        {
            this->handleTX();
        }
        // check if we are receving and the FIFO is almost full
        if (this->state == STATE_RX)
        {
            this->handleRX();
        }
    }
}

bool Si4463::send(Data &data)
{
    // encode the data
    this->m.encode(&data);

    // uint8_t cIntArgs[3] = {0, 0, 0};
    // uint8_t rIntArgs[8] = {};
    // sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);

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
        this->m.fill(this->buf, this->length)->decode(&data);
        this->length = 0; // need to reset length, info should now be stored in m
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
    this->setProperty(G_MODEM, P_MODEM_MOD_TYPE, mod);
    this->setProperty(G_MODEM, P_MODEM_MAP_CONTROL, 0x00);

    // figure out which band to use

    // parallel arrays to find correct enum
    Si4463Band bandConfigs[6] = {BAND_150, BAND_225, BAND_300, BAND_450, BAND_600, BAND_900};
    float bandDivs[6] = {24.0, 16.0, 12.0, 8.0, 6.0, 4.0};
    int bands[6] = {150, 225, 300, 450, 600, 900};
    int bandBases[6] = {144, 286, 352, 422, 572, 852}; // all are arbitrary except 422 is 2Mhz above bottom of 70cm ham band

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
        return;
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

            // now we know what band we're in, but we need to tune to the band base frequency and then use channels
            // to get the frequency we want
            double baseFreq = bandBases[band];
            // find the channel (100kHz channel step size)
            this->channel = (uint8_t)(((double)freq / 1e6 - baseFreq) / 0.1);
            // set the internal frequency to the actual frequency tuned to (may not be the set frequency since step size is 0.1MHz)
            this->freq = (baseFreq + (double)this->channel * 0.1) * 1e6;

            // see API reference FREQ_CONTROL_INTE for math
            int combinedIntFrac = baseFreq * 1e6 / (2 * (int)30e6 / (int)bandDivs[band]);               // we want to truncate to find the integer part
            fInt = combinedIntFrac - 1;                                                                 // subtract one since fraction part is between 1-2
            float intFracFraction = baseFreq * 1e6 / (2 * 30e6 / bandDivs[band]) - combinedIntFrac + 1; // don't want to truncate to get fractional part
            fFrac = intFracFraction * pow(2, 19);                                                       // from datasheet math, may truncate, but this is as close as we can get
            break;
        }
    }

    // set frequency
    // make sure we found the band and computed fInt and fFrac
    if (band != -1 && !(fInt == 0 && fFrac == 0))
    {
        this->setProperty(G_MODEM, P_MODEM_CLKGEN_BAND, bandConfigs[band]);
        uint8_t freqArgs[4] = {(uint8_t)(fInt & 0b01111111)};                // first bit must be 0
        fFrac &= 0x00FFFFFF;                                                 // first byte must be 0
        to_bytes(fFrac, 1, 1, freqArgs);                                     // put fFrac into freqArgs
        this->setProperty(G_FREQ_CONTROL, 4, P_FREQ_CONTROL_INTE, freqArgs); // set FREQ_CONTROL_INTE and FREQ_CONTROL_FRAC
    }
    else
    {
        Serial.println("ERROR: could not find frequency");
        return; // Error: could not find frequency
    }

    // set 100kHz channel step size
    uint8_t stepSizeArgs[] = {0x1b, 0x4f};
    this->setProperty(G_FREQ_CONTROL, 2, P_FREQ_CONTROL_CHANNEL_STEP_SIZE2, stepSizeArgs);

    // set data rate
    uint8_t drArgs[7] = {};
    to_bytes(dataRate, 0, 1, drArgs);
    this->setProperty(G_MODEM, 7, P_MODEM_DATA_RATE3, drArgs);

    // set frequency deviation, modulation index = 0.5
    // see datasheet for math, this includes everything but a bitrate dependent factor and 4-FSK correction
    float fDev = (double)pow(2, 19) * (double)bandDivs[band] / (2.0 * 30e6);
    if (mod == MOD_4FSK || mod == MOD_4GFSK)
        // we want the inner deviation when we are using 4-FSK but still need to multiply by 3 for some reason
        // quote from API docs: "For 4(G)FSK mode (if supported), the specified value is the inner deviation"
        // could be referring to the specified inner deviation (see below), with the need to multiply by 3 implied
        // The calculations here line up with what WDS says, so theoretically they are correct
        fDev *= 3.0;
    switch (dataRate)
    {
    case DR_500b:
        fDev *= (500.0 / 4.0);               // this adds the desired frequency to fdev, should be dataRate / 2
        this->byteDelay = 1 / (500.0 / 8e3); // ms/byte
        break;
    case DR_4_8k:
        fDev *= (4800.0 / 4.0);
        this->byteDelay = 1 / (4800.0 / 8e3);
        break;
    case DR_9_6k:
        fDev *= (9600.0 / 4.0);
        this->byteDelay = 1 / (9600.0 / 8e3);
        break;
    case DR_40k:
        fDev *= (40e3 / 4.0);
        this->byteDelay = 1 / (40e3 / 8e3);
        break;
    case DR_100k:
        fDev *= (100e3 / 4.0);
        this->byteDelay = 1 / (100e3 / 8e3);
        break;
    case DR_120k:
        fDev *= (120e3 / 4.0);
        this->byteDelay = 1 / (120e3 / 8e3);
        break;
    case DR_250k:
        fDev *= (250e3 / 4.0);
        this->byteDelay = 1 / (250e3 / 8e3);
        break;
    case DR_500k:
        fDev *= (500e3 / 4.0);
        this->byteDelay = 1 / (500e3 / 8e3);
        break;
    default:
        Serial.println("ERROR: could not find data rate");
        return; // Error: data rate is not part of the Si4463DataRate enum (should never happen)
    }

    // first 7 bits should be 0
    uint8_t fDevArgs[3] = {};
    // add 0.5 to fDev for quick rounding trick
    to_bytes((uint32_t)(fDev + 0.5) & 0x0001FFFF, 0, 1, fDevArgs);
    this->setProperty(G_MODEM, 3, P_MODEM_FREQ_DEV3, fDevArgs);

    // sets AFC to provide feedback to the PLL (does not turn on AFC)
    this->setProperty(G_MODEM, P_MODEM_AFC_MISC, 0b11111000);

    this->setProperty(G_MODEM, P_MODEM_RAW_SEARCH2, 0x84);
    this->setProperty(G_FREQ_CONTROL, P_FREQ_CONTROL_VCOCNT_RX_ADJ, 0xFE);
}

void Si4463::setPower(uint8_t pwr)
{
    // setProperty(G_PA, P_PA_MODE, 0b000001000); // this is the default
    this->setProperty(G_PA, P_PA_PWR_LVL, pwr & 0b01111111); // first bit should be 0
}

void Si4463::setPacketConfig(Si4463Mod mod, uint8_t preambleLength, uint8_t preambleThreshold)
{
    // set preamble configuration
    this->setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG, 0b00110001);
    // set preamble length
    this->setProperty(G_PREAMBLE, P_PREAMBLE_TX_LENGTH, preambleLength);
    // set preamble threshold
    this->setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG_STD_1, preambleThreshold & 0b01111111); // first bit must be 0
    // set modulation dependent packet properties
    uint8_t pktConfArgs = 0b00000000;
    uint8_t pktFieldConfArgs = 0b00000000;
    // need to enable 4 FSK at packet handler and field level and in sync words
    if (this->mod == MOD_4FSK || this->mod == MOD_4GFSK)
    {
        pktConfArgs |= 0b00100000;
        pktFieldConfArgs |= 0b00010000;
        // set sync to 4-level if needed
        this->setProperty(G_SYNC, P_SYNC_CONFIG, 0x09);
    }
    // setup all packet fields
    this->setProperty(G_PKT, P_PKT_CONFIG1, pktConfArgs);
    // turn on data whitening for fields 1 and 2
    this->setProperty(G_PKT, P_PKT_FIELD_1_CONFIG, 0x06 | pktConfArgs);
    this->setProperty(G_PKT, P_PKT_FIELD_2_CONFIG, 0x02 | pktConfArgs);

    // enable variable length packets
    this->setProperty(G_PKT, P_PKT_LEN, 0b00001010);
    this->setProperty(G_PKT, P_PKT_LEN_FIELD_SOURCE, 0x01);
    // turn off crc
    this->setProperty(G_PKT, P_PKT_CRC_CONFIG, 0x00);
    // set the length of field 1 to be 2 bytes
    uint8_t lengthFieldLen1[2] = {0x00, 0x02};
    this->setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, lengthFieldLen1);
    // set the length of field 2 to be 0 bytes, ensures we stop transmitting after field 1
    uint8_t lengthFieldLen2[2] = {0x1f, 0xff};
    this->setProperty(G_PKT, 2, P_PKT_FIELD_2_LENGTH2, lengthFieldLen2);
    // set the length of field 3 to be 0 bytes, ensures we stop transmitting after field 2
    uint8_t lengthFieldLen3[2] = {0x00, 0x00};
    this->setProperty(G_PKT, 2, P_PKT_FIELD_3_LENGTH2, lengthFieldLen3);
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
                           (uint8_t)(PIN_DO_NOTHING | pullupMask), (uint8_t)(PIN_DO_NOTHING | pullupMask), 0x00};

    // irq can only be set to specific values, if we were given a valid value, update irq in the array here
    if (irqMode < 0x04 || irqMode == 0x07 || irqMode == 0x08 || irqMode == 0x0B || irqMode == 0x0C ||
        (irqMode >= 0x0F && irqMode <= 0x1B) || irqMode == 0x1D || irqMode == 0x1F || irqMode == 0x27)
    {
        gpioArgs[4] = irqMode | pullupMask;
    }

    uint8_t resArgs[7] = {};
    this->sendCommand(C_GPIO_PIN_CFG, 7, gpioArgs, 7, resArgs);
}

void Si4463::setFRRs(Si4463FRR regAMode, Si4463FRR regBMode, Si4463FRR regCMode, Si4463FRR regDMode)
{
    // update the behavior of each register individually
    if (regAMode != FRR_NO_CHANGE)
        this->setProperty(G_FRR_CTL, P_FRR_CTL_A_MODE, regAMode);
    if (regBMode != FRR_NO_CHANGE)
        this->setProperty(G_FRR_CTL, P_FRR_CTL_B_MODE, regBMode);
    if (regCMode != FRR_NO_CHANGE)
        this->setProperty(G_FRR_CTL, P_FRR_CTL_C_MODE, regCMode);
    if (regDMode != FRR_NO_CHANGE)
        this->setProperty(G_FRR_CTL, P_FRR_CTL_D_MODE, regDMode);
}

void Si4463::setTXThreshold(uint8_t size)
{
    // limit size to 64 bytes
    if (size > 64)
        size = 64;
    this->setProperty(G_PKT, P_PKT_TX_THRESHOLD, size);
}

void Si4463::setRXThreshold(uint8_t size)
{
    // limit size to 64 bytes
    if (size > 64)
        size = 64;
    this->setProperty(G_PKT, P_PKT_RX_THRESHOLD, size);
}

void Si4463::setAFC(bool enabled)
{
    // get existing config
    uint8_t afcGainArgs[2] = {};
    this->getProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, afcGainArgs);
    // modify to enable or disable AFC
    // set or unset first bit
    if (enabled)
        afcGainArgs[0] = afcGainArgs[0] | 0b10000000;
    if (!enabled)
        afcGainArgs[0] = afcGainArgs[0] & 0b01111111;
    // apply new config
    this->setProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, afcGainArgs);
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

bool Si4463::shutdown(bool shutdown)
{
    if (shutdown)
    {
        digitalWrite(this->_sdn, HIGH);
        this->useSPICTS = true;
    }
    else
    {
        digitalWrite(this->_sdn, LOW);
        uint32_t start = millis();
        while (!this->gpio1() && millis() - start < 10)
        {
            yield();
        }
        if (millis() - start >= 10)
        {
            Serial.print("ERROR: chip failed to wake up, GPIO1 state is ");
            Serial.println(this->gpio1());
            return false;
        }
        this->spi_write(C_NOP, 0, {});
        start = millis();
        while (!this->gpio1() && millis() - start < 100)
        {
            yield();
        }
        if (millis() - start >= 100)
        {
            Serial.print("ERROR: chip did not respond to SPI, GPIO1 state is ");
            Serial.println(this->gpio1());
            return false;
        }
    }
    return true;
}

int Si4463::readFRR(int index)
{
    // call readFRRs, but only return the value of the first index in the array
    uint8_t data[4] = {0};
    this->readFRRs(data, index);
    return data[0];
}

void Si4463::setProperty(Si4463Group group, Si4463Property start, uint8_t data)
{
    // three args plus length of the data
    uint8_t cmdArgs[3 + 1] = {group, 1, start, data};
    this->sendCommandC(C_SET_PROPERTY, 3 + 1, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, Si4463Property start, uint8_t &data)
{
    // three command args, reading 1 byte of data
    uint8_t cmdArgs[3] = {group, 1, start};
    this->sendCommand(C_GET_PROPERTY, 3, cmdArgs, 1, &data);
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

    this->sendCommandC(C_SET_PROPERTY, 3 + num, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data)
{
    // make sure we're not exceeding the max number of properties we can get at a time
    if (num > MAX_NUM_PROPS)
        return;
    // three args, reading num bytes of data
    uint8_t cmdArgs[3] = {group, num, start};
    this->sendCommand(C_GET_PROPERTY, 3, cmdArgs, num, data);
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

    this->waitCTS();
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
    // must wait for CTS before sending power up command
    this->waitCTS();

    uint8_t BOOT_OPTIONS = 0b00000001;
#ifndef RF4463F30
    uint8_t XTAL_OPTIONS = 0b00000001; // assume external crystal (need to change if we have no external crystal)
#else
    uint8_t XTAL_OPTIONS = 0b00000000; // rf4463 doesn't have external crystal
#endif
    uint32_t XO_FREQ = 0x01C9C380; // 30000000 (30 MHz)

    // assmble power up options
    uint8_t options[6] = {
        BOOT_OPTIONS,
        XTAL_OPTIONS,
    };
    to_bytes(XO_FREQ, 2, 0, options);

    this->sendCommandC(C_POWER_UP, 6, options);
}

void Si4463::performIRCAL()
{
    // TODO: need to apply special config from WDS before running this
    // Note: may need to perform again with signficant change in temp

    // enter RX mode
    uint8_t rxArgs[7] = {this->channel, 0, 0, 0, 0, 0b00000011, 0b00000001};
    this->spi_write(C_START_RX, sizeof(rxArgs), rxArgs);

    this->waitCTS();

    // first calibration
    uint8_t ircal0[] = {0x56, 0x10, 0xCA, 0xF0};
    this->spi_write(C_IRCAL, sizeof(ircal0), ircal0);

    // ircal can take up to 250 ms
    this->waitCTS(300);

    // second calibration
    uint8_t ircal1[] = {0x13, 0x10, 0xCA, 0xF0};
    this->spi_write(C_IRCAL, sizeof(ircal1), ircal1);

    // ircal can take up to 250 ms
    this->waitCTS(300);
}

void Si4463::waitCTS(uint32_t timeout)
{
    // blocking while loop (should yield to other functions)
    uint32_t start = millis();
#if (FORCE_SPI_CTS == 1)
    while (!this->checkCTS() && millis() - start < timeout)
    {
        delayMicroseconds(10);
        yield();
    }
#else
    // make sure we only use GPIO CTS if it has been properly configured
    if (!useSPICTS)
        while (!this->CTS() && millis() - start < timeout)
        {
            yield();
        }
    else
        while (!this->checkCTS() && millis() - start < timeout)
        {
            delayMicroseconds(10);
            yield();
        }
#endif
    if (millis() - start >= timeout)
    {
        Serial.print("ERROR: CTS timeout");
    }
}

bool Si4463::checkCTS()
{
    // use READ_CMD_BUFF command to check the value of CTS
    // CS low through entire SPI command

    // SPI version
    digitalWrite(this->_cs, LOW);

    // send the command
    this->spi->transfer(C_READ_CMD_BUFF);
    uint8_t cts = this->spi->transfer(0x00);

    digitalWrite(this->_cs, HIGH);
    return cts == 0xff;
}

bool Si4463::CTS()
{
    // GPIO version
    if (this->_cts != -1)
    {
        return digitalRead(this->_cts);
    }
    Serial.print("ERROR: CTS pin not configured: ");
    Serial.println(this->_cts);
    return false;
}

void Si4463::sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with its args
    this->spi_write(cmd, argcCmd, argvCmd);
    // read command response
    this->spi_read(argcRes, argvRes);
}

void Si4463::sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with no args
    this->spi_write(cmd, 0, {});
    // read command response
    this->spi_read(argcRes, argvRes);
}

void Si4463::sendCommandC(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd)
{
    // send the cmd with its args
    this->spi_write(cmd, argcCmd, argvCmd);
    // need to wait for cts in case other commands are called after
    this->waitCTS();
}

void Si4463::setRadioConfig(const uint8_t *config, uint32_t length)
{
    // copy config into internal array
    this->WDS_CONFIG = new uint8_t[length];
    memcpy(this->WDS_CONFIG, config, length);
    this->configLen = length;
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
    uint32_t start = millis();
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
        if (millis() - start > CTS_TIMEOUT)
        {
            Serial.println("ERROR: spi_read(), CTS took too long");
            return;
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

void Si4463::to_bytes(uint32_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
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

void Si4463::to_bytes(uint64_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB)
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

void Si4463::applyRadioConfig()
{
    // TODO: maybe add checking?
    // if user has not specified their own config, use default
    if (this->configLen == 0)
    {
        this->applyWDSConfig();
    }
    // make sure user config has been properly set up before using it
    else if (this->configLen > 0 && this->WDS_CONFIG != nullptr)
    {
        this->applyWDSConfig(false);
    }
    else
    {
        Serial.println("ERROR: could not find correct header file for selected config");
    }
}

void Si4463::applyWDSConfig(bool applyDefault)
{
    if (applyDefault)
    {
        // read one byte (length byte), then read that many bytes, then repeat
        for (unsigned int i = 0; i < sizeof(DEFAULT_CONFIG_ARR); i++)
        {
            // CS low through entire SPI command
            digitalWrite(this->_cs, LOW);

            for (int j = 0; j < DEFAULT_CONFIG_ARR[i]; j++)
            {
                char str[5] = {};
                snprintf(str, 5, "%#02x", DEFAULT_CONFIG_ARR[i + j + 1]);
                Serial.print(str);
                Serial.print(" ");
                this->spi->transfer(DEFAULT_CONFIG_ARR[i + j + 1]);
            }
            Serial.println();
            digitalWrite(this->_cs, HIGH);
            i += DEFAULT_CONFIG_ARR[i];

            this->waitCTS();
        }
    }
    else
    {
        // read one byte (length byte), then read that many bytes, then repeat
        for (unsigned int i = 0; i < this->configLen; i++)
        {
            // CS low through entire SPI command
            digitalWrite(this->_cs, LOW);

            for (int j = 0; j < this->WDS_CONFIG[i]; j++)
            {
                this->spi->transfer(this->WDS_CONFIG[i + j + 1]);
            }
            digitalWrite(this->_cs, HIGH);
            i += this->WDS_CONFIG[i];

            this->waitCTS();
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