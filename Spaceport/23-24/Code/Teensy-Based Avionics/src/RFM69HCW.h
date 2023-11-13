#ifndef RFM69HCW_H
#define RFM69HCW_H

#include <Arduino.h>
#include "Radio.h"
#include "RFM69.h"

class RFM69HCW : public Radio
{
public:
    RFM69HCW(HardwareSerial &s, int frequency);
    void begin();
    bool tx(String message);
    String rx();
    bool encode(String &message, int type);
    bool decode(String &message, int type);
};

#endif // RFM69HCW_H