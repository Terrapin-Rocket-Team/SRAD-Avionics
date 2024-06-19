#ifndef APRS_CMD_MSG_H
#define APRS_CMD_MSG_H

#include "APRSMsgBase.h"

struct APRSCmdData final
{
    int MinutesUntilPowerOn;       // Minutes until the Pi turns on - 0 means turn on now
    int MinutesUntilVideoStart;    // Minutes until the Pi starts recording video - 0 means start now
    int MinutesUntilDataRecording; // Minutes until the FC starts recording data - 0 means start now
    bool Launch;                   // Send the launch command to the FC - 0 means don't send
};

class APRSCmdMsg final : public APRSMsgBase 
{
public:
    APRSCmdMsg(APRSHeader header);
    APRSCmdData data;

protected:
    void decodeData(int cursor) override;
    void encodeData(int cursor) override;
} ;

#endif // APRS_CMD_MSG_H