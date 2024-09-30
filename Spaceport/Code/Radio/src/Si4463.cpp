#include "Si4463.h"

/*struct Si4463HardwareConfig
{
    Si4463Mod mod;
    Si4463DataRate dataRate;
    uint32_t freq;
    uint8_t pwr;
};

struct Si4463PinConfig
{
    SPIClass spi;
    uint8_t cs;

    uint8_t sdn;
    uint8_t irq;

    uint8_t gpio0;
    uint8_t gpio1;
    uint8_t gpio2;
    uint8_t gpio3;
};*/

Si4463::Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig)
{
    this->mod = hConfig.mod;
    this->dataRate = hConfig.dataRate;
    this->freq = hConfig.freq;
    this->pwr = hConfig.pwr;

    this->spi = pConfig.spi;
    this->_cs = pConfig.cs;

    this->_gp0 = pConfig.gpio0;
    this->_gp1 = pConfig.gpio1;
    this->_gp2 = pConfig.gpio2;
    this->_gp3 = pConfig.gpio3;
}

bool Si4463::begin()
{
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
    // set power level (127 = 20 dBm)
    this->setPower(127);

    return true;
}

void Si4463::setPower(uint8_t pwr)
{
    const uint8_t PA_MODE = 0b000001000; // this is the default
    // setProperty(G_PA, P_PA_MODE, PA_MODE);
    setProperty(G_PA, P_PA_PWR_LVL, pwr & 0b01111111); // first bit should be 0
}

void Si4463::setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq)
{
    // set modulation
    setProperty(G_MODEM, P_MODEM_MOD_TYPE, MOD_CW);

    // figure out which band to use

    // parallel arrays to find correct enum
    uint8_t bandConfigs[6] = {BAND_150, BAND_225, BAND_300, BAND_450, BAND_600, BAND_900};
    float bandDivs[6] = {24.0, 16.0, 12.0, 8.0, 6.0, 4.0};
    int bands[6] = {150, 225, 300, 450, 600, 900};

    int freqBand = freq / 1e6; // truncated due to integer division
    // index of correct band
    int band = -1;
    uint8_t fInt = -1;
    uint32_t fFrac = -1;
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
    if (band != -1 && fInt != -1 && fFrac != -1)
    {
        setProperty(G_MODEM, P_MODEM_CLKGEN_BAND, bandConfigs[band]);
        uint8_t freqArgs[4] = {fInt & 0b01111111};                     // first bit must be 0
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
    if (MSB)
    {
        for (int i = 1; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            val += arr[pos++] << (i * 8);
        }
    }
}

void Si4463::from_bytes(uint32_t &val, uint8_t pos, uint8_t *arr, bool MSB)
{
    if (MSB)
    {
        for (int i = 3; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            val += arr[pos++] << (i * 8);
        }
    }
}

void Si4463::from_bytes(uint64_t &val, uint8_t pos, uint8_t *arr, bool MSB)
{
    if (MSB)
    {
        for (int i = 7; i >= 0; i--)
        {
            val += arr[pos++] << (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            val += arr[pos++] << (i * 8);
        }
    }
}

void Si4463::to_bytes(uint16_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    if (MSB)
    {
        for (int i = 1; i >= 0; i--)
        {
            arr[pos++] = (val & (0x00ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            arr[pos++] = (val & (0x00ff << (i * 8))) >> (i * 8);
        }
    }
}

void Si4463::to_bytes(uint32_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    if (MSB)
    {
        for (int i = 3; i >= 0; i--)
        {
            arr[pos++] = (val & (0x000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            arr[pos++] = (val & (0x000000ff << (i * 8))) >> (i * 8);
        }
    }
}

void Si4463::to_bytes(uint64_t val, uint8_t pos, uint8_t *arr, bool MSB)
{
    if (MSB)
    {
        for (int i = 7; i >= 0; i--)
        {
            arr[pos++] = (val & (0x00000000000000ff << (i * 8))) >> (i * 8);
        }
    }
    else
    {
        for (int i = 0; i < 8; i++)
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