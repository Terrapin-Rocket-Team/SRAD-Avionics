#include "Pi.h"
#include "RecordData/Logger.h"
using namespace mmfs;
Pi::Pi(int pinControl, int pinVideo)
{
    this->pinCmd = pinControl;
    this->pinResp = pinVideo;

    pinMode(pinControl, OUTPUT);
    pinMode(pinVideo, OUTPUT);

    digitalWrite(pinVideo, HIGH); // Set video pin to high (off) by default

    recReqst = false;
    recAkn = false;
}

void Pi::startRec(){
    if(recReqst)
        return;
    recReqst = true;
    recAkn = false;
    digitalWrite(this->pinCmd, HIGH);
    getLogger().recordLogData(INFO_, "Recording start requested.");
}

void Pi::stopRec(){
    if(!recReqst)
        return;
    recReqst = false;
    recAkn = false;
    digitalWrite(this->pinCmd, LOW);
    getLogger().recordLogData(INFO_, "Recording stop requested.");
}

bool Pi::isRecording()
{
    return digitalRead(this->pinResp) == HIGH;
}

void Pi::check()
{
    if(!recAkn && recReqst && isRecording())
    {
        recAkn = true;
        getLogger().recordLogData(INFO_, "Recording start acknowledged.");
    }
    else if(!recAkn && !recReqst && !isRecording())
    {
        recAkn = true;
        getLogger().recordLogData(INFO_, "Recording stop acknowledged.");
    }
}