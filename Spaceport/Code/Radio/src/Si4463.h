#ifndef SI4463_H
#define SI4463_H

#include "Radio.h"
#include "Si4463_defs.h"
#include "SPI.h"

#define PART_NO 0x4463
#define MAX_NUM_PROPS 12

struct Si4463HardwareConfig
{
    Si4463Mod mod;
    Si4463DataRate dataRate;
    uint32_t freq;
    uint8_t pwr;
    uint8_t preambleLen;
    uint8_t preambleThresh;
};

struct Si4463PinConfig
{
    SPIClass *spi;
    uint8_t cs;

    uint8_t sdn;
    uint8_t irq;

    uint8_t gpio0;
    uint8_t gpio1;
    uint8_t gpio2;
    uint8_t gpio3;
};

class Si4463 : public Radio
{
public:
    static const uint16_t maxLen = 0x1FFF;
    Si4463State state = STATE_IDLE;
    Message m;
    // message variables
    uint8_t buf[maxLen];
    uint16_t length = 0;
    uint16_t xfrd = 0;
    int rssi = 100;
    // hardware config
    Si4463Mod mod;
    Si4463DataRate dataRate;
    uint32_t freq;
    uint8_t pwr;
    uint8_t preambleLen;
    uint8_t preambleThresh;

    Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig);

    // radio class methods
    bool begin() override;
    bool tx(const uint8_t *message, int len = -1) override;
    bool rx() override;
    bool send(Data &data) override;
    bool receive(Data &data) override;
    int RSSI() override;

    // tx/rx helper functions
    void update(); // should be called every loop()
    void handleTX();
    void handleRX();
    bool avail();

    // interface methods
    // even higher level
    void setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq);
    void setPower(uint8_t pwr); // see datasheet for correspondence between this value and actual power output
    void setPins(Si4463Pin gpio0Mode, Si4463Pin gpio1Mode, Si4463Pin gpio2Mode, Si4463Pin gpio3Mode, Si4463Pin irqMode, bool pullup = true);
    void setFRRs(Si4463FRR regAMode, Si4463FRR regBMode = FRR_NO_CHANGE, Si4463FRR regCMode = FRR_NO_CHANGE, Si4463FRR regDMode = FRR_NO_CHANGE);
    void setAFC(bool enabled);
    bool gpio0();
    bool gpio1();
    bool gpio2();
    bool gpio3();
    bool irq();
    int readFRR(int index);

    // higher level
    void setProperty(Si4463Group group, Si4463Property start, uint8_t data);
    void getProperty(Si4463Group group, Si4463Property start, uint8_t &data);
    void setProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data);
    void getProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data);
    void readFRRs(uint8_t data[4], uint8_t start = 0);
    void powerOn();
    // slightly higher level
    void waitCTS();
    void sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes);
    void sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes);
    void sendCommandC(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd);
    bool checkCTS();

private:
    // spi interface
    SPIClass *spi;
    uint8_t _cs;
    uint8_t _irq;

    // gpio pins
    uint8_t _gp0;
    uint8_t _gp1;
    uint8_t _gp2;
    uint8_t _gp3;

    // low level abstractions
    void spi_write(uint8_t reg, uint8_t argc, uint8_t *argv);
    void spi_read(uint8_t argc, uint8_t *argv);

    static void from_bytes(uint16_t &val, uint8_t pos, uint8_t *arr, bool MSB = true);
    static void from_bytes(uint32_t &val, uint8_t pos, uint8_t *arr, bool MSB = true);
    static void from_bytes(uint64_t &val, uint8_t pos, uint8_t *arr, bool MSB = true);
    static void to_bytes(uint16_t val, uint8_t pos, uint8_t *arr, bool MSB = true);
    static void to_bytes(uint32_t val, uint8_t pos, uint8_t *arr, bool MSB = true);
    static void to_bytes(uint64_t val, uint8_t pos, uint8_t *arr, bool MSB = true);
};

#endif