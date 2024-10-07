#include "Si4463.h"

Si4463::Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig)
{
    this->mod = hConfig.mod;
    this->dataRate = hConfig.dataRate;
    this->freq = hConfig.freq;
    this->pwr = hConfig.pwr;
    this->preambleLen = hConfig.preambleLen;
    this->preambleThresh = hConfig.preambleThresh;

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
    // set pins
    pinMode(_cs, OUTPUT);
    pinMode(_irq, INPUT);
    pinMode(_gp0, INPUT);
    pinMode(_gp1, INPUT);
    pinMode(_gp2, INPUT);
    pinMode(_gp3, INPUT);

    // complete power on sequence
    this->powerOn();

    // check part info
    uint8_t args[8] = {0};
    this->sendCommandR(C_PART_INFO, 8, args);

    uint16_t partNo = 0;
    from_bytes(partNo, 1, args);
    if (partNo != PART_NO)
        return false; // make sure proper communication has been established

    // apparently this needs to be set manually
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
    setProperty(G_PKT, P_PKT_LEN, 0b00011010);
    setProperty(G_PKT, P_PKT_LEN_FIELD_SOURCE, 0b00000001);
    setProperty(G_PKT, P_PKT_TX_THRESHOLD, 10); // fire interrupt when 10 bytes in FIFO
    setProperty(G_PKT, P_PKT_RX_THRESHOLD, 54); // fire interrupt when 54 bytes in FIFO
    uint8_t lengthFieldLen[2] = {0, 4};
    setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, lengthFieldLen);
    uint8_t dataMaxLen[2] = {0};
    to_bytes(this->maxLen, 0, dataMaxLen);
    setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, dataMaxLen);

    return true;
}

bool Si4463::tx(const uint8_t *message, int len)
{
    if (len > this->maxLen)
        return false;

    this->length = len;
    memcpy(this->buf, message, this->length);
    // prefill fifo in idle state
    if (this->state == STATE_IDLE)
    {
        digitalWrite(this->_cs, LOW);

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
    if (this->state == STATE_TX)
    {
        digitalWrite(this->_cs, LOW);

        this->spi->transfer(C_WRITE_TX_FIFO);

        while (!gpio1() && this->xfrd < this->length)
        {
            this->spi->transfer(this->buf[this->xfrd++]);
        }

        digitalWrite(this->_cs, HIGH);

        if (this->xfrd > this->length)
            return; // should never be here
        if (this->xfrd == this->length)
        {
            // automatically placed into an idle state
            this->state = STATE_IDLE;
            memset(this->buf, 0, this->length);
            this->length = 0;
            this->xfrd = 0;
        }
    }
}

bool Si4463::rx()
{
    if (this->state == STATE_IDLE)
    {
        uint8_t rxArgs[7] = {0, 0, 0, 0, 0, 0b00000011, 0b00000011};
        spi_write(C_START_RX, 7, rxArgs);
        this->state = STATE_ENTER_RX;
        return true;
    }
    return false;
}

void Si4463::handleRX()
{
    if (this->state == STATE_RX)
    {
        digitalWrite(this->_cs, LOW);

        this->spi->transfer(C_READ_RX_FIFO);

        if (this->length == 0 && this->xfrd == 0)
        {
            // first message, need to read length
            uint8_t mLen[2] = {0};
            mLen[0] = this->spi->transfer(0x00);
            mLen[1] = this->spi->transfer(0x00);
            from_bytes(length, 0, mLen);
            if (length > this->maxLen)
            {
                length = 0;
                return; // error, message too long
            }
        }

        while (!gpio0() && this->xfrd < this->length)
        {
            this->buf[this->xfrd++] = this->spi->transfer(0x00);
        }

        digitalWrite(this->_cs, HIGH);

        if (this->xfrd > this->length)
            return; // should never be here
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

    if (gpio0() && this->state == STATE_TX)
    {
        handleTX();
    }
    if (gpio1() && this->state == STATE_RX)
    {
        handleRX();
    }
    if (irq() && (this->state == STATE_RX || this->state == STATE_RX_COMPLETE))
    {
        this->rssi = readFRR(1);
    }
}

bool Si4463::send(Data &data)
{
    this->m.encode(&data);
    return this->tx(m.buf, m.size);
}

bool Si4463::receive(Data &data)
{
    if (this->state == STATE_RX_COMPLETE)
    {
        m.fill(this->buf, this->length)->decode(&data);
        return true;
    }
    return false;
}

int Si4463::RSSI() { return this->rssi / 2 - 64 - 70; } // magic formula from datasheet

bool Si4463::avail()
{
    if (this->state == STATE_IDLE)
        return this->rx();
    else
        return this->state == STATE_RX_COMPLETE;
}

void Si4463::setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq)
{
    // set modulation
    setProperty(G_MODEM, P_MODEM_MOD_TYPE, mod);

    // set modulation dependant properties
    uint8_t pktConfArgs = 0b00000000;
    uint8_t pktFieldConfArgs = 0b00000000;
    // need to enable 4 level at packet handler and field level
    if (mod == MOD_4FSK || mod == MOD_4GFSK)
    {
        pktConfArgs |= 0b01000000;
        pktFieldConfArgs |= 0b00010000;
    }
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

    int freqBand = freq / 1e6; // truncated due to integer division
    // index of correct band
    int band = -1;
    uint8_t fInt = 0;
    uint32_t fFrac = 0;
    for (int i = 0; i < 6; i++)
    {
        if (freqBand > bands[i])
        {
            if (band < 5)
            {
                int lowerDiff = freq - bands[i - 1] * 1e6;
                int upperDiff = -1 * (freq - bands[i] * 1e6);
                if (lowerDiff < upperDiff)
                    band = i - 1;
                else
                    band = i;

                // see API reference FREQ_CONTROL_INTE for math
                int combinedIntFrac = freq / (2 * (int)30e6 / (int)bandDivs[band]);           // we want to truncate
                fInt = combinedIntFrac - 1;                                                   // subtract one since fraction part is between 1-2
                float intFracFraction = freq / (2 * 30e6 / bandDivs[band]) - combinedIntFrac; // don't want to truncate to get fractional part
                fFrac = intFracFraction * pow(2, 19);                                         // from math, may truncate, but this is as close as we can get
            }
            else
            {
                return; // error frequency too low
            }
        }
    }

    // set frequency
    if (band != -1 && !(fInt == 0 && fFrac == 0))
    {
        setProperty(G_MODEM, P_MODEM_CLKGEN_BAND, bandConfigs[band]);
        uint8_t freqArgs[4] = {(uint8_t)(fInt & 0b01111111)};          // first bit must be 0
        fFrac &= 0x00FFFFFF;                                           // first byte must be 0
        to_bytes(fFrac, 1, freqArgs);                                  // put fFrac into freqArgs
        setProperty(G_FREQ_CONTROL, 4, P_FREQ_CONTROL_INTE, freqArgs); // set FREQ_CONTROL_INTE and FREQ_CONTROL_FRAC
    }

    // set data rate
    uint8_t drArgs[8] = {};
    to_bytes(dataRate, 0, drArgs);
    setProperty(G_MODEM, 7, P_MODEM_DATA_RATE3, drArgs);

    // set frequency deviation
    float fDev = (float)pow(2, 19) * bandDivs[band] / (float)(2 * 30e6);
    if (mod == MOD_4FSK || mod == MOD_4GFSK)
        fDev /= 3.0; // should be the inner deviation for 4FSK
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
        return; // error data rate is not part of the Si4463DataRate enum (should never happen)
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
    if (pullup)
    {
        pullupMask = 0b01000000;
    }
    uint8_t gpioArgs[7] = {(uint8_t)(gpio0Mode | pullupMask), (uint8_t)(gpio1Mode | pullupMask),
                           (uint8_t)(gpio2Mode | pullupMask), (uint8_t)(gpio3Mode | pullupMask),
                           (uint8_t)(PIN_DO_NOTHING | pullupMask), (uint8_t)(PIN_SDO | pullupMask), 0x00};

    if (irqMode < 0x04 || irqMode == 0x07 || irqMode == 0x08 || irqMode == 0x0B || irqMode == 0x0C ||
        (irqMode >= 0x0F && irqMode <= 0x1B) || irqMode == 0x1D || irqMode == 0x1F || irqMode == 0x27)
    {
        gpioArgs[4] = irqMode | pullupMask;
    }

    sendCommandC(C_GPIO_PIN_CFG, 7, gpioArgs);
}

void Si4463::setFRRs(Si4463FRR regAMode, Si4463FRR regBMode, Si4463FRR regCMode, Si4463FRR regDMode)
{
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
    uint8_t afcGainArgs[2] = {0b1000011, 0x69}; // default
    if (!enabled)
    {
        afcGainArgs[0] = 0b0000011;
        afcGainArgs[1] = 0x69;
    }
    setProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, afcGainArgs);
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
    uint8_t data[4] = {0};
    readFRRs(data, index);
    return data[0];
}

void Si4463::setProperty(Si4463Group group, Si4463Property start, uint8_t data)
{
    uint8_t cmdArgs[3 + 1] = {group, 1, start, data};
    sendCommandC(C_SET_PROPERTY, 3 + 1, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, Si4463Property start, uint8_t &data)
{
    uint8_t cmdArgs[3] = {group, 1, start};
    sendCommand(C_SET_PROPERTY, 3, cmdArgs, 1, &data);
}

void Si4463::setProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data)
{
    if (num > MAX_NUM_PROPS)
        return;
    uint8_t cmdArgs[3 + num] = {group, num, start};
    for (int i = 0; i < num; i++)
        cmdArgs[3 + i] = data[i];

    sendCommandC(C_SET_PROPERTY, 3 + num, cmdArgs);
}

void Si4463::getProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data)
{
    if (num > MAX_NUM_PROPS)
        return;
    uint8_t cmdArgs[3] = {group, num, start};
    sendCommand(C_SET_PROPERTY, 3, cmdArgs, num, data);
}

void Si4463::readFRRs(uint8_t data[4], uint8_t start)
{
    // CS needs to stay low throughout reading FRRs
    digitalWrite(this->_cs, LOW);

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
    waitCTS();

    uint8_t BOOT_OPTIONS = 0b00000000;
    uint8_t XTAL_OPTIONS = 0b00000001; // assume external crystal
    uint32_t XO_FREQ = 0x01C9C380;     // 30000000 (30 MHz)

    uint8_t options[6] = {
        BOOT_OPTIONS,
        XTAL_OPTIONS,
    };
    to_bytes(XO_FREQ, 2, options);

    sendCommandC(C_POWER_UP, 6, options);
}

// used when the command has args and a response besides CTS
void Si4463::sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes)
{
    spi_write(cmd, argcCmd, argvCmd);
    waitCTS();
    spi_read(argcRes, argvRes);
}

// used when the command has no args but has a response besides CTS
void Si4463::sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes)
{
    spi_write(cmd, 0, {});
    waitCTS();
    spi_read(argcRes, argvRes);
}

// used when the command has args but no response besides CTS
void Si4463::sendCommandC(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd)
{
    spi_write(cmd, argcCmd, argvCmd);
    waitCTS();
}

void Si4463::waitCTS()
{
    while (!checkCTS())
    {
        yield();
    }
}

bool Si4463::checkCTS()
{
    uint8_t args[1] = {0};
    sendCommandR(C_READ_CMD_BUFF, 1, args);
    return args[0] == 0xff;
}

// private methods

void Si4463::spi_write(uint8_t cmd, uint8_t argc, uint8_t *argv)
{
    digitalWrite(this->_cs, LOW);

    this->spi->transfer(cmd);
    for (int i = 0; i < argc; i++)
    {
        this->spi->transfer(argv[i]);
    }

    digitalWrite(this->_cs, HIGH);
}

void Si4463::spi_read(uint8_t argc, uint8_t *argv)
{
    digitalWrite(this->_cs, LOW);

    uint8_t pos = 0;

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
            arr[pos++] = (val & (0x00ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < iterNum; i++)
        {
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

int pow(int base, int exponent)
{
    int val = base;
    exponent--;
    while (exponent--)
        val *= base;
    return val;
}