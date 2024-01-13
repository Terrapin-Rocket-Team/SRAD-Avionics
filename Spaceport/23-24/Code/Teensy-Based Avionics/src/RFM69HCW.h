#ifndef RFM69HCW_H
#define RFM69HCW_H

#define IS_RFM69HW_HCW

#include <Arduino.h>
#include "Radio.h"
#include "RFM69.h"
#include "APRS-Decoder.h"
#include "APRSEncodeFunctions.h"

class RFM69HCW : public Radio
{
public:
    RFM69HCW(uint32_t frequency, bool transmitter, bool highBitrate, APRSConfig config);
    void begin(SPIClass *s, uint8_t cs, uint8_t irq, int frqBand);
    bool tx(String message);
    String rx();
    bool encode(String &message, EncodingType type);
    bool decode(String &message, EncodingType type);
    bool send(String message, EncodingType type);
    String receive(EncodingType type);
    bool available();
    int RSSI();

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
    char *buf;
    uint8_t bufSize = RF69_MAX_DATA_LEN;
    APRSConfig cfg;
    bool avail;
    int lastRSSI;
    char *lastMsg;
};

#endif // RFM69HCW_H