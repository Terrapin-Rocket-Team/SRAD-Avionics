#ifndef SI4463_H
#define SI4463_H

#include "Radio.h"
#include "Si4463_defs.h"
#include "SPI.h"

// include the default configuration file
#include "si4463_default.h"
// RF4463F30 COTS radios require this to be defined
#define RF4463F30
// set to 1 to always force using SPI for CTS
#define FORCE_SPI_CTS 0

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
    static const uint16_t MAX_LEN = 0x1FFF;
    // Si4463 part number
    static const uint16_t PART_NO = 0x4463;
    // maximum number of properties in a single get/setProperties call
    static const uint8_t MAX_NUM_PROPS = 12;
    // timeout for waiting for CTS
    static const uint8_t CTS_TIMEOUT = 100; // ms
    // length of the FIFO in default config
    static const uint8_t FIFO_LENGTH = 129; // bytes
    // RX_FIFO_FULL interrupt occurs when there are more than RX_THRESH bytes in FIFO
    static const uint8_t RX_THRESH = 63; // bytes (max 64)
    // TX_FIFO_EMPTY interrupt occurs when there is more than TX_THRESH bytes of space in FIFO
    static const uint8_t TX_THRESH = 63; // bytes (max 64)
    // the current radio state, does not always align with hardware state
    Si4463State state = STATE_IDLE;
    // a Message object used to encode and decode the message
    Message m;
    // the buffer to store messages that are currently being sent or received
    uint8_t buf[Si4463::MAX_LEN];
    // the length of the buffer
    uint16_t length = 0;
    // the number of message bytes transferred
    uint16_t xfrd = 0;
    // uint32_t userWRLen = 0;
    // the received signal strength
    int rssi = 100;
    // the modulation scheme the radio is using
    Si4463Mod mod;
    // the current radio symbol rate
    Si4463DataRate dataRate;
    // the current transmit/receive frequency = baseFreq + channel * channelSpacing
    uint32_t freq;
    // the current channel used to set the transmit/receive frequency
    uint8_t channel = 0;
    // the current transmit power (0-127), see datasheet
    uint8_t pwr;
    // the current preamble length in symbols
    uint8_t preambleLen;
    // the current preamble length threshold for packet detection in symbols
    uint8_t preambleThresh;
    // whether a full message has been received
    bool available = false;
    // the amount of time between attempting to read/write bytes
    uint32_t byteDelay = 0;

    /*
    Si4463 constructor, uses the following default configuration
    Hardware:
    - mod: MOD_2GFSK
    - dataRate: DR_100k
    - freq: 433 MHz
    - pwr: 127 (+20dBm)
    - preambleLength: 48
    - preambleThresh: 16
    Pin Config:
    - spi: SPI
    - cs: 10
    - sdn: 38
    - irq: 33
    - gpio0: 34
    - gpio1: 35
    - gpio2: 36
    - gpio3: 37
    */
    Si4463();
    /*
    Si4463 constructor
    - hConfig : the hardware configuration to initialize the radio with
    - pConfig : the pin configuration to use when communicating with this radio
    */
    Si4463(Si4463HardwareConfig hConfig, Si4463PinConfig pConfig);

    // Destructor
    ~Si4463();

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

    // radio class functionality extensions
    /*
    Wraps setting a user specified WDS config before normal begin(), rather than using the default
    - config : the configuration array (from header file)
    - length : the length of the configuration array
    */
    bool begin(const uint8_t *config, uint32_t length);
    // TODO:
    // void writeTXBuf(const uint8_t *data, int length);
    // void readRXBuf(const uint8_t *data, int length);

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
    Sets the packet structure and processing configuration of the radio
    - mod : sets the modulation used for the packet (needed to switch between 2 and 4 level FSK)
    - preambleLength : sets the length of the transmitted preamble
    - preambleThreshold : sets the required preamble length received without error
    */
    void setPacketConfig(Si4463Mod mod, uint8_t preambleLength, uint8_t preambleThresh);
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
    Sets the amount of empty space needed in the FIFO for TX_FIFO_EMPTY to go high
    - size : the amount of empty space in bytes (max 64)
    */
    void setTXThreshold(uint8_t size);
    /*
    Sets the numbers of bytes needed in the FIFO for RX_FIFO_FULL to go high
    - size : the number of bytes (max 64)
    */
    void setRXThreshold(uint8_t size);
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
    /*
    Controls the power state of the radio and executes the Power On Reset (POR) sequence
    - shutdown : whether the radio should be shutdown (true) or powered up (false)
    Returns: whether the radio was successfully put into the desired state
    */
    bool shutdown(bool shutdown);
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
    Sets properties by reading sequentially from an array containing the entire command sequence.
    Unused helper overload to set properties from Si446X library.
    - data : the command sequence
    - size : the length of ```data```
    */
    void setProperty(uint8_t *data, uint8_t size);
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
    /*
    Completes the Image Rejection Calibration sequence, currently non-functional
    */
    void performIRCAL();

    // lower level functions to directly send radio commands
    /*
    Blocks until the Clear to Send (CTS) byte is received
    */
    void waitCTS(uint32_t timeout = Si4463::CTS_TIMEOUT);
    /*
    Checks for the Clear to Send (CTS) byte using SPI
    Returns: whether the CTS byte was sent
    */
    bool checkCTS();
    /*
    Checks for the Clear to Send (CTS) byte using GPIO, returns false if CTS pin is not set
    Returns: whether the CTS byte was sent
    */
    bool CTS();
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
    /*
    Set a user specified WDS config, rather than using the default
    - config : the configuration array (from header file)
    - length : the length of the configuration array
    */
    void setRadioConfig(const uint8_t *config, uint32_t length);
    /*
    Checks whether to apply the default settings or user specified settings and applies them
    - mod : sets the modulation type
    - dataRate : sets the symbol rate
    - freq : sets the frequency (Hz)
    */
    void applyRadioConfig();

private:
    SPIClass *spi;
    uint8_t _cs;
    // spi interface
    uint8_t _sdn;
    uint8_t _irq;

    // gpio pins
    uint8_t _gp0;
    uint8_t _gp1;
    uint8_t _gp2;
    uint8_t _gp3;

    // other pins (these will be set to one of the above gpio pins)
    int _cts = -1;

    // WDS radio config variables
    // array to hold WDS config
    uint8_t *WDS_CONFIG = nullptr;
    // length of WDS_CONFIG
    uint32_t configLen = 0;

    // other config that can be private
    // timer for byteDelay
    uint32_t timer = millis();
    // force use of SPI CTS until GPIO is setup
    bool useSPICTS = true;

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
    /*
    Applies the config from WDS from a header file generated by compileHeaders.py
    */
    void applyWDSConfig(bool applyDefault = true);
};

#endif