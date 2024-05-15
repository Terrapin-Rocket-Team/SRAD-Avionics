#ifndef RFM69HCW_H
#define RFM69HCW_H

#include "RH_RF69.h"
#include "Radio.h"
#include "APRSMsg.h"

#define RADIO_BUFFER_SIZE 1024
#define RADIO_FLAG_MORE_DATA 0b00000001 // custom flag to indicate more packets are still to be sent

/*
Settings:
- double frequency in Mhz
- bool transmitter
- bool highBitrate
- RHGenericSPI *spi pointer to radiohead SPI object
- uint8_t cs CS pin
- uint8_t irq IRQ pin
- uint8_t rst RST pin
*/
struct RadioSettings
{
    double frequency;
    bool transmitter;
    bool highBitrate;
    RHGenericSPI *spi;
    uint8_t cs;
    uint8_t irq;
    uint8_t rst;
    int txPower = 20;
};

#ifndef RH_RF69_MAX_MESSAGE_LEN
#define RH_RF69_MAX_MESSAGE_LEN 66
#endif // !RH_RF69_MAX_MESSAGE_LEN

class RFM69HCWNew : public Radio
{
public:
    RFM69HCWNew(const RadioSettings *s);
    bool init() override;
    bool tx(const uint8_t *message, int len = -1, int packetNum = 0, bool lastPacket = true) override; // designed to be used internally. cannot exceed 66 bytes including headers
    bool rx(uint8_t *recvbuf, uint8_t *len) override;
    bool busy();
    bool enqueueSend(RadioMessage *message) override;               // designed to be used externally. can exceed 66 bytes.
    bool enqueueSend(const char *message) override;                 // designed to be used externally. can exceed 66 bytes.
    bool enqueueSend(const uint8_t *message, uint8_t len) override; // designed to be used externally. can exceed 66 bytes.

    bool dequeueReceive(RadioMessage *message) override; // designed to be used externally. can exceed 66 bytes.
    bool dequeueReceive(char *message) override;         // designed to be used externally. can exceed 66 bytes.
    bool dequeueReceive(uint8_t *message) override;      // designed to be used externally. can exceed 66 bytes.
    int RSSI() override;
    void set300KBPSMode();
    bool update();

    explicit operator bool() const { return initialized; }

    // increment the specified index, wrapping around if necessary
    static void inc(int &i) { i = (i + 1) % RADIO_BUFFER_SIZE; }

    uint8_t buffer[RADIO_BUFFER_SIZE];
    RH_RF69 radio;

private:
    // all radios should have the same networkID
    const uint8_t networkID = 1;
    // default to the highest transmit power
    const int txPower = 20;
    // set by constructor
    RadioSettings settings;
    int thisAddr;
    int toAddr;
    bool avail;
    int rssi;
    bool initialized = false;

    // stores queue of messages, with the length of the message as the first byte of each. no delimiter.
    // [6xxxxxx3xxx1x2xx]
    // Done to allow for messages that are not ascii encoded.
    int bufHead = 0;
    int bufTail = 0;
    int remainingLength = 0;   // how much message is left to send for transmitters
    int orignalBufferTail = 0; // used to update the length of the message after recieving.
    const int maxDataLen = RH_RF69_MAX_MESSAGE_LEN;
};

#endif // RFM69HCW_H