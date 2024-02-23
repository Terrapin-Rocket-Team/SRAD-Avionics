#ifndef RFM69HCW_H
#define RFM69HCW_H

#define IS_RFM69HW_HCW

#include "Radio.h"
#include "APRSMsg.h"
#include "RH_RF69.h"
#include "APRSEncodeFunctions.h"

/*
double frequency in Mhz
bool transmitter
bool highBitrate
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
};

class RFM69HCW : public Radio
{
public:
    RFM69HCW(RadioSettings s, APRSConfig config);
    ~RFM69HCW();
    bool begin() override;
    bool tx(char *message) override;
    const char *rx() override;
    bool encode(char *message, EncodingType type) override;
    bool decode(char *message, EncodingType type) override;
    bool send(const char *message, EncodingType type) override;
    const char *receive(EncodingType type) override;
    bool available();
    int RSSI() override;
    RH_RF69 radio;

private:
    // all radios should have the same networkID
    const uint8_t networkID = 0x0001;
    // default to the highest transmit power
    const int txPower = 20;
    // set by constructor
    uint16_t addr;
    uint16_t toAddr;
    RadioSettings settings;
    // for sending/receiving data
    char buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t bufSize = RH_RF69_MAX_MESSAGE_LEN;
    APRSConfig cfg;
    bool avail;
    int avgRSSI;
    int incomingMsgLen;
    char *lastMsg;
};

#endif // RFM69HCW_H