#include "Si4463.h"

bool Si4463::begin()
{
    // complete power on sequence
    powerOn();

    // check part info
    uint8_t args[8] = {0};
    spi_read(C_PART_INFO, 8, args);

    uint16_t partNo = 0;
    from_bytes(partNo, 1, args);
    if (partNo != PART_NO)
        return false;

    return true;
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

    spi_write(C_POWER_UP, 6, options);

    waitCTS();
}

void Si4463::waitCTS()
{
    while (spi_read(C_READ_CMD_BUFF) != 0xff)
    {
        yield();
    }
}

void Si4463::spi_write(uint8_t reg, uint8_t argc, uint8_t *argv)
{
    digitalWrite(this->_cs, LOW);

    this->spi->transfer(reg);
    for (int i = 0; i < argc; i++)
    {
        this->spi->transfer(argv[i]);
    }

    digitalWrite(this->_cs, HIGH);
}

uint8_t Si4463::spi_read(uint8_t reg)
{
    uint8_t args[1] = {0};
    spi_read(reg, 1, args);
    return args[0];
}

void Si4463::spi_read(uint8_t reg, uint8_t argc, uint8_t *argv)
{
    digitalWrite(this->_cs, LOW);

    uint8_t pos = 0;

    this->spi->transfer(reg);

    waitCTS();

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