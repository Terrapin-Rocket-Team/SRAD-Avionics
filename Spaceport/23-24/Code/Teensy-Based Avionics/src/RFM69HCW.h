#ifndef RFM69HCW_H
#define RFM69HCW_H

#define IS_RFM69HW_HCW

#include "Radio.h"
#include "APRSMsg.h"
#include "RFM69.h"
#include "APRSEncodeFunctions.h"

class RFM69HCW : public Radio
{
public:
    RFM69HCW(uint32_t frequency, bool transmitter, bool highBitrate, APRSConfig config);
    ~RFM69HCW();
    void begin() override;
    void begin(SPIClass *s, uint8_t cs, uint8_t irq, uint8_t rst);
    bool tx(char *message) override;
    const char *rx() override;
    bool encode(char *message, EncodingType type) override;
    bool decode(char *message, EncodingType type) override;
    bool send(const char *message, EncodingType type) override;
    const char *receive(EncodingType type) override;
    bool available();
    int RSSI() override;

private:
    RFM69 radio;
    // all radios should have the same networkID
    const uint8_t networkID = 0x0001;
    // default to the highest transmit power
    const int txPower = 20;
    // set by constructor
    uint16_t addr;
    uint16_t toAddr;
    uint32_t frq;
    bool isTransmitter;
    bool isHighBitrate;
    // for sending/receiving data
    char buf[RF69_MAX_DATA_LEN];
    uint8_t bufSize = RF69_MAX_DATA_LEN;
    APRSConfig cfg;
    bool avail;
    int avgRSSI;
    int incomingMsgLen;
    char *lastMsg;
};

#endif // RFM69HCW_H