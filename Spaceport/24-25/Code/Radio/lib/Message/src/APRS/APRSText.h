#ifndef APRSTEXT_H
#define APRSTEXT_H

#include "APRSData.h"

class APRSText : public APRSData
{
public:
    static const int maxMsgLen = 68;
    static const int maxAddrLen = 10;
    char msg[68] = {0};       // 67 message length + 1 for \0
    char addressee[10] = {0}; // 9 addressee length + 1 for \0
    int msgLen = 0;
    int addrLen = 0;
    // constructors
    APRSText(APRSConfig config);
    // the char arrays are expected to be valid c strings
    APRSText(APRSConfig config, char msg[67], char addressee[9]);

    // encode/decode

    // encode the data stored in the ```Data``` object and place the result in ```data```
    uint16_t encode(uint8_t *data, uint16_t sz) override;
    // decode the data stored in ```data``` and place it in the ```Data``` object
    uint16_t decode(uint8_t *data, uint16_t sz) override;
};

#endif