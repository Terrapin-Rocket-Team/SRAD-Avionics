#ifndef RADIODRIVER_H
#define RADIODRIVER_H

#include <Arduino.h>

// tx/rx states
enum RadioState : uint8_t
{
    STATE_IDLE,        // not doing anything
    STATE_ENTER_TX,    // chip commanded to enter TX mode
    STATE_TX,          // in the middle of TX
    STATE_TX_COMPLETE, // finished transferring bytes to the radio
    STATE_ENTER_RX,    // chip commanded to enter RX mode
    STATE_RX,          // in the middle of RX
    STATE_RX_COMPLETE, // finished RX
};

class RadioDriver
{
public:
    // the current radio state, does not always align with hardware state
    RadioState state;

    virtual ~RadioDriver() {}; // Virtual descructor. Very important
    virtual bool begin() = 0;
    virtual bool tx(const uint8_t *message, uint16_t len = -1) = 0;
    virtual bool rx(uint8_t *data, uint16_t *len, uint16_t maxLen) = 0;
    /*
    Similar to tx(), but starts transmitting without all the bytes available yet
    Remaining bytes need to be made available via writeTXBuf()
    - data : the data to start transmitting with
    - len : the length of the data
    - totalLen : the total length of data to be transmitted (including bytes yet to be made available)
    Returns: whether a transmission was successfully started
    */
    virtual bool startTX(const uint8_t *data, uint16_t len, uint16_t totalLen) = 0;

    virtual bool startRx() = 0;
    /*
    Used in conjunction with startTX() to make remaining bytes in the transmission available
    - data : the data to add to the transmission
    - len : the length of the data
    Returns: the length of data successfully added
    */
    virtual uint16_t writeTXBuf(const uint8_t *data, uint16_t len) = 0;
    /*
    Similar to writeTXBuf(), can be used to read from the RX buffer before the full message has been received
    - data : the array to read data into
    - len : the length of data to read
    Returns: the length of data successfully added
    */
    virtual uint16_t readRXBuf(uint8_t *data, uint16_t len) = 0;
    /*
    Polling update function, must be called every loop without significant blocking delays
    */
    virtual void update() = 0;
    /*
    Used to check if data is available to be retrieved using receive(), also places the radio into rx mode
    Returns: the length of data if a full message is available, otherwise 0
    */
    virtual uint32_t avail() = 0;
    virtual int RSSI() = 0;
};

#endif // RADIO_H