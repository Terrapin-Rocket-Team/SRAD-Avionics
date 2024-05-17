#ifndef RFM69HCW_H
#define RFM69HCW_H

#include "RH_RF69.h"
#include "Radio.h"
#include "APRS/APRSMsgBase.h"

const int TRANSCEIVER_MESSAGE_BUFFER_SIZE = 512; // length of the buffer underpinning the queue of messages to send or receive. Radio makes 2 buffers this size for sending and receiving.
#define RADIO_FLAG_MORE_DATA 0b00000001          // custom flag to indicate more packets are still to be sent

#ifndef RH_RF69_MAX_MESSAGE_LEN
#define RH_RF69_MAX_MESSAGE_LEN 66
#endif // !RH_RF69_MAX_MESSAGE_LEN

struct buffer
{
    uint8_t data[TRANSCEIVER_MESSAGE_BUFFER_SIZE];
    int head;
    int tail;
};

class RFM69HCW : public Radio
{
public:
    RFM69HCW(const RadioSettings *s);
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
    bool update() override;

    explicit operator bool() const { return initialized; }

    // increment the specified index, wrapping around if necessary
    static void inc(int &i) { i = (i + 1) % TRANSCEIVER_MESSAGE_BUFFER_SIZE; }

    // public during debugging

    // stores queue of messages, with the length of the message as the first byte of each. no delimiter.
    // [6xxxxxx3xxx1x2xx]
    // Done to allow for messages that are not ascii encoded.
    buffer sendBuffer;
    buffer recvBuffer;
    RH_RF69 radio;

private:
    // all radios should have the same networkID
    const uint8_t networkID = 1;

    int thisAddr;
    int toAddr;
    int rssi;
    bool initialized = false;

    int remainingLength = 0;   // how much message is left to send for transmitters
    int orignalBufferTail = 0; // used to update the length of the message after recieving.

    void copyToBuffer(buffer &buffer, const uint8_t *src, int len);
    void copyFromBuffer(buffer &buffer, uint8_t *dest, int len, bool pop = true);
};

#endif // RFM69HCW_H