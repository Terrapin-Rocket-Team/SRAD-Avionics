#include "APRSCmdMsg.h"

void APRSCmdMsg::encodeData(int cursor)
{
    snprintf((char *)&string[cursor], RADIO_MESSAGE_BUFFER_SIZE - cursor,
             ":%-9s:PWR:%d VID:%d DAT:%d L:%d", header.CALLSIGN, data.MinutesUntilPowerOn, data.MinutesUntilVideoStart, data.MinutesUntilDataRecording, data.Launch);
    len = strlen((char *)string);
}
void APRSCmdMsg::decodeData(int cursor)
{
    sscanf((char *)&string[cursor], ":%*9s:PWR:%d VID:%d DAT:%d L:%d", &data.MinutesUntilPowerOn, &data.MinutesUntilVideoStart, &data.MinutesUntilDataRecording, &data.Launch);
}

APRSCmdMsg::APRSCmdMsg(APRSHeader &header) : APRSMsgBase(header)
{
    data.MinutesUntilPowerOn = 0;
    data.MinutesUntilVideoStart = 0;
    data.MinutesUntilDataRecording = 0;
    data.Launch = false;
}