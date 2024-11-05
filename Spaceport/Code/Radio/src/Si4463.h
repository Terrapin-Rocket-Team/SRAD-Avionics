#ifndef SI4463_H
#define SI4463_H

#include "Radio.h"
#include "Si4463_defs.h"
#include "SPI.h"

#define PART_NO 0x4463
#define MAX_NUM_PROPS 12

/*
Si4463 Hardware Configuration
- Si4463Mod mod : the modulation scheme (2FSK, 4FSK, etc)
- Si4463DataRate dataRate : the symbol rate (40k, 120k, etc)
- uint32_t freq : the transmit/receive frequency in Hz
- uint8_t pwr : the transmit power (0-127), see datasheet
- uint8_t preambleLen : the length of the preamble in symbols
- uint8_t preambleThresh : preamble length threshold for packet detection in symbols
*/
struct Si4463HardwareConfig
{
    Si4463Mod mod;
    Si4463DataRate dataRate;
    uint32_t freq;
    uint8_t pwr;
    uint8_t preambleLen;
    uint8_t preambleThresh;
};

/*
Si4463 Pin Configuration
- SPIClass *spi : a pointer to the SPI object to use
- uint8_t cs : the MCU pin connected to the chip select pin
- uint8_t sdn : the MCU pin connected to the shutdown pin
- uint8_t irq : the MCU pin connected to the interrupt pin
- uint8_t gpio0 : the MCU pin connected to GPIO 0
- uint8_t gpio1 : the MCU pin connected to GPIO 1
- uint8_t gpio2 : the MCU pin connected to GPIO 2
- uint8_t gpio3 : the MCU pin connected to GPIO 3
*/
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
    // the maximum length of a transmitted or received message
    static const uint16_t maxLen = 0x1FFF;
    // the current radio state, does not reflect hardware state
    Si4463State state = STATE_IDLE;
    // a Message object used to encode and decode the message
    Message m;
    // the buffer to store messages that are currently being sent or received
    uint8_t buf[maxLen];
    // the length of the buffer
    uint16_t length = 0;
    // the number of message bytes transferred
    uint16_t xfrd = 0;
    // the received signal strength
    int rssi = 100;
    // the modulation scheme the radio is using
    Si4463Mod mod;
    // the current radio symbol rate
    Si4463DataRate dataRate;
    // the current transmit/receive frequency
    uint32_t freq;
    // the current transmit power (0-127), see datasheet
    uint8_t pwr;
    // the current preamble length in symbols
    uint8_t preambleLen;
    // the current preamble length threshold for packet detection in symbols
    uint8_t preambleThresh;

    /*
    Si4463 constructor
    - hConfig : the hardware configuration to initialize the radio with
    - pConfig : the pin configuration to use when communicating with this radio
    */
    Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig);

    // radio class funtions
    /*
    Begins communication with the radio, initializes and checks SPI communication and sets hardware confirguration
    Returns: whether communication was successfully established
    */
    bool begin() override;
    /*
    Base transmit method, called to start a new transmission
    - message : the message to be transmitted
    - len : the length of the message
    Returns: whether a transmission was successfully started
    */
    bool tx(const uint8_t *message, int len = -1) override;
    /*
    Base receive method, called to set the radio to receive mode
    Returns: whether the radio was successfully placed into receive mode
    */
    bool rx() override;
    /*
    Send a message using the data in ```data```
    - data : the ```Data``` object containing the message to be sent
    Returns: whether the transmission was successfully started
    */
    bool send(Data &data) override;
    /*
    Receive a message and place it in ```data```, should only be called after avail() returns true
    - data : the ```Data``` object to place the message data in
    Returns: whether a message was successfully received
    */
    bool receive(Data &data) override;
    /*
    Get recevied signal strength in dBm
    Returns: the recevied signal strength in dBm
    */
    int RSSI() override;

    // tx/rx helper functions
    /*
    Polling update function, must be called every loop without significant blocking delays
    */
    void update();
    /*
    Function called by update() during transmit
    */
    void handleTX();
    /*
    Function called by update() during receive
    */
    void handleRX();
    /*
    Used to check if data is available to be retrieved using receive(), also places the radio into rx mode
    Returns: whether a new message is available
    */
    bool avail();

    // high level hardware configuration methods
    /*
    Sets important modem configuration properties for the radio, mostly related to factors that affect the radio wave
    - mod : sets the modulation type
    - dataRate : sets the symbol rate
    - freq : sets the frequency (Hz)
    */
    void setModemConfig(Si4463Mod mod, Si4463DataRate dataRate, uint32_t freq);
    /*
    Sets the transmit power of the radio, see datasheet for correspondence between this value and actual power output
    - pwr : the power level of the radio (0-127)
    */
    void setPower(uint8_t pwr);
    /*
    Sets the behavior of configurable radio pins
    - gpio0Mode : sets the behavior of GPIO 0
    - gpio1Mode : sets the behavior of GPIO 1
    - gpio2Mode : sets the behavior of GPIO 2
    - gpio3Mode : sets the behavior of GPIO 3
    - irqMode : sets the behavior of NIRQ
    - pullup : sets whether the pins should be connected to an internal pullup resistor
    */
    void setPins(Si4463Pin gpio0Mode, Si4463Pin gpio1Mode, Si4463Pin gpio2Mode, Si4463Pin gpio3Mode, Si4463Pin irqMode, bool pullup = true);
    /*
    Sets the behavrio of configurable Fast Response Registers (FRRs)
    - regAMode : sets the value stored in FRR A
    - regBMode : sets the value stored in FRR B, does not modify the register if not passed
    - regCMode : sets the value stored in FRR C, does not modify the register if not passed
    - regDMode : sets the value stored in FRR D, does not modify the register if not passed
    */
    void setFRRs(Si4463FRR regAMode, Si4463FRR regBMode = FRR_NO_CHANGE, Si4463FRR regCMode = FRR_NO_CHANGE, Si4463FRR regDMode = FRR_NO_CHANGE);
    /*
    Used to enable or disable the radio's Automatic Frequency Control (AFC)
    - enabled : whether the AFC should be enabled
    */
    void setAFC(bool enabled);
    /*
    Used to get the state of the GPIO 0 pin
    Returns: the state of GPIO 0
    */
    bool gpio0();
    /*
    Used to get the state of the GPIO 1 pin
    Returns: the state of GPIO 1
    */
    bool gpio1();
    /*
    Used to get the state of the GPIO 2 pin
    Returns: the state of GPIO 2
    */
    bool gpio2();
    /*
    Used to get the state of the GPIO 3 pin
    Returns: the state of GPIO 3
    */
    bool gpio3();
    /*
    Used to get the state of the IRQ pin
    Returns: the state of IRQ
    */
    bool irq();

    void shutdown(bool shutdown);
    /*
    Used to read the state of only one FRR
    - index : the FRR to read from
    Returns: the state of the FRR specified by ```index```
    */
    int readFRR(int index);

    // high level functions to use various radio commands
    /*
    Sets a single property on the radio
    - group : the property group the property belongs to
    - start : the individual property to set
    - data : the value to set the property to
    */
    void setProperty(Si4463Group group, Si4463Property start, uint8_t data);
    /*
    Gets a single property from the radio
    - group : the property group the property belongs to
    - start : the individual property to get
    - data : the variable to be updated with the property value
    */
    void getProperty(Si4463Group group, Si4463Property start, uint8_t &data);
    /*
    Sets a multiple properties on the radio
    - group : the property group the properties belong to
    - num : the number of properties to set
    - start : the starting index of the properties to set
    - data : the values to set the properties to
    */
    void setProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data);
    /*
    Gets multiple properties from the radio
    - group : the property group the properties belongs to
    - num : the number of properties to get
    - start : the starting index of the properties to get
    - data : the variable to be updated with the property values
    */
    void getProperty(Si4463Group group, const uint8_t num, Si4463Property start, uint8_t *data);
    /*
    Reads all the FRRs
    - data : the array to be populated with the values of the FRRs
    - start : the index of the FRR to start reading at
    */
    void readFRRs(uint8_t data[4], uint8_t start = 0);
    /*
    Completes the power on sequence for the radio, must be called to use the radio after exiting from a shutdown state
    */
    void powerOn();
    // lower level functions to directly send radio commands
    /*
    Blocks until the Clear to Send (CTS) byte is received
    */
    void waitCTS();
    /*
    Checks for the Clear to Send (CTS) byte
    Returns: whether the CTS byte was sent
    */
    bool checkCTS();
    /*
    Send a command to the radio, used when the command has arguments and a response besides CTS
    - cmd : the command to send
    - argcCmd : the number of command arguments
    - argvCmd : the values of the command arguments
    - argcRes : the number of response arguments
    - argvRes : the values of the response arguments
    */
    void sendCommand(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd, uint8_t argcRes, uint8_t *argvRes);
    /*
    Send a command to the radio, used when the command has no arguments but has a response besides CTS
    - cmd : the command to send
    - argcRes : the number of response arguments
    - argvRes : the values of the response arguments
    */
    void sendCommandR(Si4463Cmd cmd, uint8_t argcRes, uint8_t *argvRes);
    /*
    Send a command to the radio, used when the command has arguments but no response besides CTS
    - cmd : the command to send
    - argcCmd : the number of command arguments
    - argvCmd : the values of the command arguments
    */
    void sendCommandC(Si4463Cmd cmd, uint8_t argcCmd, uint8_t *argvCmd);

    SPIClass *spi;
    uint8_t _cs;

private:
    // spi interface
    uint8_t _sdn;
    uint8_t _irq;

    // gpio pins
    uint8_t _gp0;
    uint8_t _gp1;
    uint8_t _gp2;
    uint8_t _gp3;

    // abstractions of low level SPI operations
    /*
    Performs an spi write operation, writing a register or command followed by arguments
    - reg : the first byte of the operation, usually a register or command
    - argc : the number of arguments
    - argv : the values of the arguments
    */
    void spi_write(uint8_t reg, uint8_t argc, uint8_t *argv);
    /*
    Performs an spi read operation, used to read information from the spi bus
    - argc : the number of bytes to read
    - argv : the values of the bytes received
    */
    void spi_read(uint8_t argc, uint8_t *argv);

    // static helper methods
    /*
    Converts the bytes stored in ```arr``` into a uint16_t
    - val : the resulting value from the array
    - pos : the position in the array to start reading from (the array must be at least of length pos + 1)
    - arr : the array to read bytes from
    - MSB : whether the bytes should be read most significant byte first
    */
    static void from_bytes(uint16_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
    /*
    Converts the bytes stored in ```arr``` into a uint32_t
    - val : the resulting value from the array
    - pos : the position in the array to start reading from (the array must be at least of length pos + 3)
    - arr : the array to read bytes from
    - MSB : whether the bytes should be read most significant byte first
    */
    static void from_bytes(uint32_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
    /*
    Converts the bytes stored in ```arr``` into a uint64_t
    - val : the resulting value from the array
    - pos : the position in the array to start reading from (the array must be at least of length pos + 7)
    - arr : the array to read bytes from
    - MSB : whether the bytes should be read most significant byte first
    */
    static void from_bytes(uint64_t &val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
    /*
    Converts a uint16_t to a series of bytes stored in ```arr```
    - val : the value to be stored in the array
    - pos : the position in the array to start writing to (the array must be at least of length pos + 1)
    - arr : the array to write bytes into
    - MSB : whether the bytes should be written most significant byte first
    */
    static void to_bytes(uint16_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
    /*
    Converts a uint32_t to a series of bytes stored in ```arr```
    - val : the value to be stored in the array
    - pos : the position in the array to start writing to (the array must be at least of length pos + 3)
    - arr : the array to write bytes into
    - MSB : whether the bytes should be written most significant byte first
    */
    static void to_bytes(uint32_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
    /*
    Converts a uint64_t to a series of bytes stored in ```arr```
    - val : the value to be stored in the array
    - pos : the position in the array to start writing to (the array must be at least of length pos + 7)
    - arr : the array to write bytes into
    - MSB : whether the bytes should be written most significant byte first
    */
    static void to_bytes(uint64_t val, uint8_t pos, uint8_t bytePos, uint8_t *arr, bool MSB = true);
};

#endif