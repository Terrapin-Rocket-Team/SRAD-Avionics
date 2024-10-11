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
    this->_cs = pConfig.cs;
    this->_irq = pConfig.irq;

    this->_gp0 = pConfig.gpio0;
    this->_gp1 = pConfig.gpio1;
    this->_gp2 = pConfig.gpio2;
    this->_gp3 = pConfig.gpio3;
}

bool Si4463::begin()
{
    // set pins for correct modes
    pinMode(_cs, OUTPUT);
    pinMode(_irq, INPUT);
    pinMode(_gp0, INPUT);
    pinMode(_gp1, INPUT);
    pinMode(_gp2, INPUT);
    pinMode(_gp3, INPUT);

    // complete power on sequence
    this->powerOn();

    // check part info to make sure proper communication has been established
    uint8_t args[8] = {0};
    this->sendCommandR(C_PART_INFO, 8, args);

    uint16_t partNo = 0;
    from_bytes(partNo, 1, args);
    if (partNo != PART_NO)
        return false; // Error: did not receive the correct part number

    // set the global config, this is the defaults, but apparently a reserved field needs to be set manually
    this->setProperty(G_GLOBAL, P_GLOBAL_CONFIG, 0b01100000);

    // set modem (frequency related) config
    this->setModemConfig(this->mod, this->dataRate, this->freq);
    // set power level (127 = ~20 dBm)
    this->setPower(this->pwr);
    // turn on AFC
    this->setAFC(true);
    // set preamble length
    setProperty(G_PREAMBLE, P_PREAMBLE_TX_LENGTH, this->preambleLen);
    // set preamble threshold
    setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG_STD_1, this->preambleThresh & 0b01111111); // first bit must be 0
    // leaving sync word params at defaults

    // set defaults for gpio pins
    this->setPins(PIN_TX_FIFO_EMPTY, PIN_RX_FIFO_FULL, PIN_RX_STATE, PIN_TX_STATE, PIN_VALID_PREAMBLE);

    // set defaults for FRRs
    this->setFRRs(FRR_CURRENT_STATE, FRR_LATCHED_RSSI, FRR_INT_STATUS, FRR_INT_CHIP_STATUS);

    // packet handling setup
    setProperty(G_PKT, P_PKT_LEN, 0b00011010);              // set received field length to be 2 bytes, leave in the length bytes, and set the variable length field to field 2
    setProperty(G_PKT, P_PKT_LEN_FIELD_SOURCE, 0b00000001); // set the length field to be field 1
    setProperty(G_PKT, P_PKT_TX_THRESHOLD, 10);             // fire interrupt when 10 bytes in FIFO
    setProperty(G_PKT, P_PKT_RX_THRESHOLD, 54);             // fire interrupt when 54 bytes in FIFO
    // set the length of the length field (field 1) to be 4 bytes
    uint8_t lengthFieldLen[2] = {0, 4};
    setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, lengthFieldLen);
    // set the length of the data field (field 2) to be the maximum data length of 8191 bytes
    uint8_t dataMaxLen[2] = {0};
    to_bytes(this->maxLen, 0, dataMaxLen);
    setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, dataMaxLen);

    return true;
}

bool Si4463::tx(const uint8_t *message, int len)
{
    // make sure the packet isn't too long
    if (len > this->maxLen)
        return false; // Error: the packet is too long

    // otherwise add the message to the internal buffer
    this->length = len;
    memcpy(this->buf, message, this->length);
    // prefill fifo in idle state
    if (this->state == STATE_IDLE)
    {
        digitalWrite(this->_cs, LOW);

        // write to TX FIFO
        this->spi->transfer(C_WRITE_TX_FIFO);

        // send length
        uint8_t mLen[2] = {0};
        to_bytes(this->length, 0, mLen);
        this->spi->transfer(mLen[0]);
        this->spi->transfer(mLen[1]);

        // send message body
        while (!gpio1() && this->xfrd < this->length)
        {
            this->spi->transfer(this->buf[this->xfrd++]);
        }

        digitalWrite(this->_cs, HIGH);

        // start tx
        uint8_t txArgs[6] = {0, 0b00110000, 0, 0, 0, 0};
        spi_write(C_START_TX, 6, txArgs);

        this->state = STATE_TX;
        return true;
    }
    return false;
}

void Si4463::handleTX()
{
    // this function assumes we are in tx mode already, so check that we are in tx mode
    if (this->state == STATE_TX)
    {
        digitalWrite(this->_cs, LOW);

        // write to the TX FIFO
        this->spi->transfer(C_WRITE_TX_FIFO);

        // write remaining data
        while (!gpio1() && this->xfrd < this->length)
        {
            this->spi->transfer(this->buf[this->xfrd++]);
        }

        digitalWrite(this->_cs, HIGH);

        // if we've sent this->length bytes, the message is complete
        if (this->xfrd == this->length)
        {
            // automatically placed into an idle state
            this->state = STATE_IDLE;
            // clear internal variables
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
        // enter RX mode
        uint8_t rxArgs[7] = {0, 0, 0, 0, 0, 0b00000011, 0b00000011};
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
        digitalWrite(this->_cs, LOW);

        // read from RX FIFO
        this->spi->transfer(C_READ_RX_FIFO);

        // if the internal length and xfrd variables are 0, then this is the first part of the message
        if (this->length == 0 && this->xfrd == 0)
        {
            // so we need to read length
            uint8_t mLen[2] = {0};
            mLen[0] = this->spi->transfer(0x00);
            mLen[1] = this->spi->transfer(0x00);
            // convert individual bytes to uint16_t
            from_bytes(this->length, 0, mLen);
            // make sure the message is not too long (could be erroneous transmission)
            if (this->length > this->maxLen)
            {
                this->length = 0;
                return; // error, message too long
            }
        }

        // recevie message data
        while (!gpio0() && this->xfrd < this->length)
        {
            this->buf[this->xfrd++] = this->spi->transfer(0x00);
        }

        digitalWrite(this->_cs, HIGH);

        // if we've transferred length bytes, we've received the whole message
        if (this->xfrd == this->length)
        {
            // automatically placed into an idle state
            this->state = STATE_RX_COMPLETE;
            // only reset xfrd
            // length and buf need to stay so they can be read
            this->xfrd = 0;
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
    if (this->state == STATE_ENTER_TX && checkCTS())
        this->state = STATE_TX;
    if (this->state == STATE_ENTER_RX && checkCTS())
        this->state = STATE_RX;

    // check if we are transmitting and the FIFO is almost empty
    if (this->state == STATE_TX && gpio0())
    {
        handleTX();
    }
    // check if we are receving and the FIFO is almost full
    if (this->state == STATE_RX && gpio1())
    {
        handleRX();
    }
    // check if we've detected a valid preamble in receive mode so we can get the rssi
    if (irq() && (this->state == STATE_RX || this->state == STATE_RX_COMPLETE))
    {
        this->rssi = readFRR(1);
    }
}

bool Si4463::send(Data &data)
{
    // encode the data
    this->m.encode(&data);
    // send the data
    return this->tx(m.buf, m.size);
}

bool Si4463::receive(Data &data)
{
    // check if we have received the whole message
    if (this->state == STATE_RX_COMPLETE)
    {
        // decode the message
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
        return this->state == STATE_RX_COMPLETE;
    return false;
}

void Si4463::setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq)
{
    // set modulation
    setProperty(G_MODEM, P_MODEM_MOD_TYPE, mod);

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

    if (!(freqBand >= 142 && freqBand <= 175) ||
        !(freqBand >= 284 && freqBand <= 350) ||
        !(freqBand >= 350 && freqBand <= 420) ||
        !(freqBand >= 420 && freqBand <= 525) ||
        !(freqBand >= 850 && freqBand <= 1050))
    {
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
            int combinedIntFrac = freq / (2 * (int)30e6 / (int)bandDivs[band]);           // we want to truncate to find the integer part
            fInt = combinedIntFrac - 1;                                                   // subtract one since fraction part is between 1-2
            float intFracFraction = freq / (2 * 30e6 / bandDivs[band]) - combinedIntFrac; // don't want to truncate to get fractional part
            fFrac = intFracFraction * pow(2, 19);                                         // from dtasheet math, may truncate, but this is as close as we can get
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
        to_bytes(fFrac, 1, freqArgs);                                  // put fFrac into freqArgs
        setProperty(G_FREQ_CONTROL, 4, P_FREQ_CONTROL_INTE, freqArgs); // set FREQ_CONTROL_INTE and FREQ_CONTROL_FRAC
    }
    else
    {
        return; // Error: could not find frequency
    }

    // set data rate
    uint8_t drArgs[8] = {};
    to_bytes(dataRate, 0, drArgs);
    setProperty(G_MODEM, 7, P_MODEM_DATA_RATE3, drArgs);

    // set frequency deviation
    // see datasheet for math, this includes everything but a bitrate dependent factor and 4-FSK correction
    float fDev = (float)pow(2, 19) * bandDivs[band] / (float)(2 * 30e6);
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

    uint8_t fDevArgs[4] = {};
    // first 7 bits should be 0
    to_bytes((uint32_t)fDev & 0x0001FFFF, 0, fDevArgs);
    setProperty(G_MODEM, 3, P_MODEM_FREQ_DEV3, fDevArgs);
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
                           (uint8_t)(PIN_DO_NOTHING | pullupMask), (uint8_t)(PIN_SDO | pullupMask), 0x00};

    // irq can only be set to specific values, if we were given a valid value, update irq in the array here
    if (irqMode < 0x04 || irqMode == 0x07 || irqMode == 0x08 || irqMode == 0x0B || irqMode == 0x0C ||
        (irqMode >= 0x0F && irqMode <= 0x1B) || irqMode == 0x1D || irqMode == 0x1F || irqMode == 0x27)
    {
        gpioArgs[4] = irqMode | pullupMask;
    }

    sendCommandC(C_GPIO_PIN_CFG, 7, gpioArgs);
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
    uint8_t afcGainArgs[2] = {0b1000011, 0x69}; // default value
    // set first bit to 0 to disable AFC
    if (!enabled)
    {
        afcGainArgs[0] = 0b0000011;
        afcGainArgs[1] = 0x69;
    }
    setProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, afcGainArgs);
    // set up the AFC if enabled
    if (enabled)
    {
        uint8_t afcLimiterArgs[8] = {0b01000000, 0xBE}; // set max AFC deviation to 190*8 = 1.52 kHz
        setProperty(G_MODEM, 2, P_MODEM_AFC_LIMITER2, afcLimiterArgs);
        uint8_t afcMiscArgs = 0b11101000; // turns on AFC feedback to the PLL
        setProperty(G_MODEM, P_MODEM_AFC_MISC, afcMiscArgs);
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

int Si4463::readFRR(int index)
{
    // call readFRRs, but only return the value of the first index in the array
    uint8_t data[4] = {0};
    readFRRs(data, index);
    return data[0];
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
    waitCTS();

    uint8_t BOOT_OPTIONS = 0b00000000;
    uint8_t XTAL_OPTIONS = 0b00000001; // assume external crystal (need to change if we have no external crystal)
    uint32_t XO_FREQ = 0x01C9C380;     // 30000000 (30 MHz)

    // assmble power up options
    uint8_t options[6] = {
        BOOT_OPTIONS,
        XTAL_OPTIONS,
    };
    to_bytes(XO_FREQ, 2, options);

    sendCommandC(C_POWER_UP, 6, options);
}

void Si4463::sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with its args
    spi_write(cmd, argcCmd, argvCmd);
    // need to wait for cts before we read
    waitCTS();
    // read command response
    spi_read(argcRes, argvRes);
}

void Si4463::sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes)
{
    // send the cmd with no args
    spi_write(cmd, 0, {});
    // need to wait for cts before we read
    waitCTS();
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
        yield();
    }
}

bool Si4463::checkCTS()
{
    // use READ_CMD_BUFF command to check the value of CTS
    uint8_t args[1] = {0};
    sendCommandR(C_READ_CMD_BUFF, 1, args);
    return args[0] == 0xff;
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
    digitalWrite(this->_cs, LOW);

    uint8_t pos = 0;

    // read in the args (CTS must already have been received)
    while (pos < argc)
    {
        argv[pos++] = this->spi->transfer(0x00);
    }

    digitalWrite(this->_cs, HIGH);
}

void Si4463::from_bytes(uint16_t &val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 2 - pos;
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

void Si4463::from_bytes(uint32_t &val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 4 - pos;
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

void Si4463::from_bytes(uint64_t &val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 8 - pos;
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

void Si4463::to_bytes(uint16_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 2 - pos;
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

void Si4463::to_bytes(uint32_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 4 - pos;
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

void Si4463::to_bytes(uint64_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    int iterNum = 8 - pos;
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