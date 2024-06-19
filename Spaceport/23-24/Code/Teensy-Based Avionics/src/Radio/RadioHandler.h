#include "RFM69HCW.h"
#include "APRS/APRSCmdMsg.h"
#include "APRS/APRSTelemMsg.h"

#include <BlinkBuzz.h>
#include <RecordData.h>

#include "../Pi.h"
#include "../State.h"

namespace radioHandler
{
    void processCmdData(APRSCmdMsg &cmd, APRSCmdData &old, APRSCmdData &currentCmdData, int time)
    {

        bb.aonoff(BUZZER, 100, 2);
        char log[100];
        if (cmd.data.Launch)
        {
            snprintf(log, 100, "Launch Command Changed to %d.", cmd.data.Launch);
            recordLogData(INFO, log);
            currentCmdData.Launch = cmd.data.Launch;
        }
        if (cmd.data.MinutesUntilPowerOn >= 0)
        {
            snprintf(log, 100, "Power On Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilPowerOn, (int)(currentCmdData.MinutesUntilPowerOn - time) / 60000);
            recordLogData(INFO, log);
            currentCmdData.MinutesUntilPowerOn = cmd.data.MinutesUntilPowerOn + time;
        }
        if (cmd.data.MinutesUntilVideoStart >= 0)
        {
            snprintf(log, 100, "Video Start Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilVideoStart, (int)(currentCmdData.MinutesUntilVideoStart - time) / 60000);
            recordLogData(INFO, log);
            currentCmdData.MinutesUntilVideoStart = cmd.data.MinutesUntilVideoStart + time;
        }
        else if(cmd.data.MinutesUntilVideoStart == -2){
            currentCmdData.MinutesUntilVideoStart = -2;
        }
        if (cmd.data.MinutesUntilDataRecording >= 0)
        {
            snprintf(log, 100, "Data Recording Time Changed: %d with %d minutes remaining.", cmd.data.MinutesUntilDataRecording, (int)(currentCmdData.MinutesUntilDataRecording - time) / 60000);
            recordLogData(INFO, log);
            currentCmdData.MinutesUntilDataRecording = cmd.data.MinutesUntilDataRecording + time;
        }
    }

    void processCurrentCmdData(APRSCmdData &currentCmdData, State &computer, Pi &rpi, int time)
    {
        if (currentCmdData.Launch && computer.getStageNum() == 0)
        {
            recordLogData(INFO, "Launch Command Received. Launching Rocket.");
            computer.launch();
        }
        if (time > currentCmdData.MinutesUntilPowerOn && !rpi.isOn())
        {
            recordLogData(INFO, "Power On RPI Time Reached. Turning on Raspberry Pi.");
            rpi.setOn(true);
        }
        if(currentCmdData.MinutesUntilVideoStart == -2 && rpi.isOn()){
            recordLogData(INFO, "Video Transmit Restart Requested. Resetting.");
            rpi.setRecording(false);
            currentCmdData.MinutesUntilVideoStart = 0;
        }
        else if (time > currentCmdData.MinutesUntilVideoStart && !rpi.isRecording() && rpi.isOn())
        {
            rpi.setRecording(true);
        }
        if (time > currentCmdData.MinutesUntilDataRecording && !computer.getRecordOwnFlightData())
        {
            recordLogData(INFO, "Data Recording Time Reached. Starting Data Recording.");
            computer.setRecordOwnFlightData(true);
        }
    }
}