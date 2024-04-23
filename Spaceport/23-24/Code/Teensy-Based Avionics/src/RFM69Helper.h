#ifndef RFM69HELPER_H
#define RFM69HELPER_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) // Windows

#elif defined(__unix__)  // Linux
// TODO
#elif defined(__APPLE__) // OSX
// TODO
#endif

#ifndef ARDUINO
#include <iostream>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <string>
#include <cstring>

#define SS 0

static uint64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

static uint32_t millis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime;
};

enum SPIDataMode
{
    SPI_MODE0
};

enum SPIBitOrder
{
    MSBFIRST,
    LSBFIRST
};

enum SPIClockDivider
{
    SPI_CLOCK_DIV2,
    SPI_CLOCK_DIV8,
    SPI_CLOCK_DIV16
};

class SPISettings
{
public:
    SPISettings(){};
    SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
    {
        this->clock = clock;
        this->bitOrder = bitOrder;
        this->dataMode = dataMode;
    }
    uint32_t clock = 4000000U;
    uint8_t bitOrder = MSBFIRST;
    uint8_t dataMode = SPI_MODE0;
};

// class SPIClass
// {
// public:
//     SPIClass();
//     void begin();

//     // with transaction
//     void beginTransaction(SPISettings settings);
//     void endTransaction();

//     // non transaction
//     void setDataMode(SPIDataMode dm);
//     void setBitOrder(SPIBitOrder bo);
//     void setClockDivider(SPIClockDivider cd);
//     uint8_t transfer(uint8_t data);
// };

// SPIClass SPI;

enum PinState : bool
{
    LOW,
    HIGH
};

enum PinMode : bool
{
    INPUT,
    OUTPUT
};

enum InterruptTrigger : bool
{
    FALLING,
    RISING
};

// void digitalWrite(int pin, PinState state);
// void pinMode(int pin, PinMode mode);
// void attachInterrupt(uint8_t pin, void (*function)(), InterruptTrigger mode);
// void detachInterrupt(uint8_t pin);

enum SerialPrintMode
{
    HEX,
    BIN
};

class HardwareSerial
{
public:
    HardwareSerial(){};
    void print(const char *s) { std::cout << s; };
    void print(uint8_t num, SerialPrintMode m)
    {
        std::ios_base::fmtflags f(std::cout.flags());
        if (m == HEX)
        {
            std::cout << std::hex << num;
            std::cout.flags(f);
        }
        if (m == BIN)
            std::cout << std::bitset<sizeof(num)>(num);
    };
    void println() { std::cout << std::endl; };
    void println(const char *s) { std::cout << s << std::endl; };
    void println(uint8_t num, SerialPrintMode m)
    {
        print(num, m);
        std::cout << std::endl;
    };
};

static HardwareSerial Serial = HardwareSerial();

#endif

// add to RFM69.h
/*
#elif defined(_WIN32) || defined(_WIN64) // Windows
#define RF69_PLATFORM RF69_PLATFORM_WINDOWS
#else
*/

// TODO define the following
/*
digitalWrite
pinMode
SPIClass and necessary functions
SPI
HIGH
LOW
OUTPUT
millis
attachInterrupt
RISING

SPI_MODE0
MSBFIRST    or SPI transaction

SPI_CLOCK_DIV2
detachInterrupt
Serial print and println
    HEX
    BIN
byte
*/
#endif