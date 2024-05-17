#include "APRSCmdMsg.h"

void APRSCmdMsg::encodeData(int cursor)
{
    snprintf((char *)&string[cursor], RADIO_MESSAGE_BUFFER_SIZE - cursor,
             ":%-09s:PWR:%d VID:%d DAT:%d L:%d", header.CALLSIGN, data.MinutesUntilPowerOn, data.MinutesUntilVideoStart, data.MinutesUntilDataRecording, data.Launch);
}
void APRSCmdMsg::decodeData(int cursor)
{
    sscanf((char *)&string[cursor + 11], "PWR:%d VID:%d DAT:%d L:%d", &data.MinutesUntilPowerOn, &data.MinutesUntilVideoStart, &data.MinutesUntilDataRecording, &data.Launch);
}

APRSCmdMsg::APRSCmdMsg(APRSHeader &header) : APRSMsgBase(header)
{
    data.MinutesUntilPowerOn = -1;
    data.MinutesUntilVideoStart = -1;
    data.MinutesUntilDataRecording = -1;
    data.Launch = false;
}