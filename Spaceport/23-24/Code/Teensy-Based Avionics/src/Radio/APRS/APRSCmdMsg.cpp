#include "APRSCmdMsg.h"

APRSCmdMsg::APRSCmdMsg(APRSHeader header) : APRSMsgBase(header)
{
    data.MinutesUntilPowerOn = -1;
    data.MinutesUntilVideoStart = -1;
    data.MinutesUntilDataRecording = -1; // -1 so that we know if the value was set or not
    data.Launch = false;
}

void APRSCmdMsg::encodeData(int cursor)
{
    snprintf((char *)&string[cursor], RADIO_MESSAGE_BUFFER_SIZE - cursor,
             ":%-9s:PWR:%d VID:%d DAT:%d L:%d",
             header.CALLSIGN,
             data.MinutesUntilPowerOn,
             data.MinutesUntilVideoStart,
             data.MinutesUntilDataRecording,
             data.Launch);
    len = strlen((char *)string);
}

void APRSCmdMsg::decodeData(int cursor)
{
    sscanf((char *)&string[cursor + 11], // ignore the :TO_FIELD: part
           "PWR:%d VID:%d DAT:%d L:%d",
           &data.MinutesUntilPowerOn,
           &data.MinutesUntilVideoStart,
           &data.MinutesUntilDataRecording,
           &data.Launch);
}