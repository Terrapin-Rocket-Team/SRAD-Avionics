#ifndef MOCKRADIO_H
#define MOCKRADIO_H

#include "Radio.h"
#include "Si4463_defs.h"
#include "SPI.h"

/*
Si4463 Hardware Configuration
- Si4463Mod mod : the modulation scheme (2FSK, 4FSK, etc)
- Si4463DataRate dataRate : the symbol rate (40k, 120k, etc)
- uint32_t freq : the transmit/receive frequency in Hz
- uint8_t pwr : the transmit power (0-127), see datasheet
- uint8_t preambleLen : the length of the preamble in symbols
- uint8_t preambleThresh : preamble length threshold for packet detection in symbols
*/
struct MockHardwareConfig
{
    int dataRate;
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
struct MockPinConfig
{
    HardwareSerial *s;
};

class MockRadio : public Radio
{
public:
    // the maximum length of a transmitted or received message
    static const uint16_t MAX_LEN = 0x1FFF;
    // length of the FIFO in default config
    static const uint8_t FIFO_LENGTH = 129; // bytes
    // RX_FIFO_FULL interrupt occurs when there are more than RX_THRESH bytes in FIFO
    static const uint8_t RX_THRESH = 40; // bytes (max 64)
    // TX_FIFO_EMPTY interrupt occurs when there is more than TX_THRESH bytes of space in FIFO
    static const uint8_t TX_THRESH = 63; // bytes (max 64)
    // the current radio state, does not always align with hardware state
    Si4463State state = STATE_IDLE;
    // a Message object used to encode and decode the message
    Message m;
    // the buffer to store messages that are currently being sent or received
    uint8_t buf[MockRadio::MAX_LEN];
    // the length of the buffer
    uint16_t length = 0;
    // the number of message bytes transferred
    uint16_t xfrd = 0;
    // pointer to the end of data in ```buf``` (TX), or the start of unread data (RX)
    uint32_t availLen = 0;
    // the current radio symbol rate
    int dataRate = 0;
    // whether a full message has been received
    bool available = false;

    uint32_t debugTimer = micros();

    /*
    MockRadio constructor, uses the following default configuration
    */
    MockRadio();
    /*
    MockRadio constructor
    - hConfig : the hardware configuration to initialize the radio with
    - pConfig : the pin configuration to use when communicating with this radio
    */
    MockRadio(MockHardwareConfig hConfig, MockPinConfig pConfig);

    // Destructor
    ~MockRadio();

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
    Similar to tx(), but starts transmitting without all the bytes available yet
    Remaining bytes need to be made available via writeTXBuf()
    - data : the data to start transmitting with
    - len : the length of the data
    - totalLen : the total length of data to be transmitted (including bytes yet to be made available)
    Returns: whether a transmission was successfully started
    */
    bool startTX(const uint8_t *data, uint16_t len, uint16_t totalLen);
    /*
    Used in conjunction with startTX() to make remaining bytes in the transmission available
    - data : the data to add to the transmission
    - len : the length of the data
    Returns: the length of data successfully added
    */
    uint16_t writeTXBuf(const uint8_t *data, uint16_t len);
    /*
    Similar to writeTXBuf(), can be used to read from the RX buffer before the full message has been received
    - data : the array to read data into
    - len : the length of data to read
    Returns: the length of data successfully added
    */
    uint16_t readRXBuf(uint8_t *data, uint16_t len);

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

private:
    HardwareSerial *s;

    // internal radio states
    bool gpio0State = 0; // TX Empty High
    bool gpio1State = 0; // RX Full High
    bool gpio2State = 0; // RX High
    bool gpio3State = 0; // TX High
    uint8_t internalBuf[129];
    uint8_t intBufP = 0;

    int totalBytes = 0;
    int internalLength = 0;

    // other config that can be private
    bool TXEmptyFlag = false;
    bool RXFullFlag = false;
    // force use of SPI CTS until GPIO is setup
    uint32_t byteTimer = micros();
    uint32_t lastUpdate = 0;

    // abstractions of low level SPI operations
    void bufWrite(uint8_t c);

    uint8_t bufRead();

    void bufClear();

    void internalUpdate();

    void setTXMode();
    void setRXMode();

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