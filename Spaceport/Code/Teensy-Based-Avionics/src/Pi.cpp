#include "Pi.h"

Pi::Pi(int pinControl, int pinVideo)
{
    this->pinControl = pinControl;
    this->pinVideo = pinVideo;

    pinMode(pinControl, OUTPUT);
    pinMode(pinVideo, OUTPUT);

    digitalWrite(pinVideo, HIGH); // Set video pin to high (off) by default

    on = false;
    recording = false;
}

void Pi::startRec(){
    if(recReqst)
        return;
    recReqst = true;
    recAkn = false;
    digitalWrite(this->pinCmd, HIGH);
    logger.recordLogData(mmfs::INFO_, "Recording start requested.");
}

void Pi::stopRec(){
    if(!recReqst)
        return;
    recReqst = false;
    recAkn = false;
    digitalWrite(this->pinCmd, LOW);
    logger.recordLogData(mmfs::INFO_, "Recording stop requested.");
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
        logger.recordLogData(mmfs::INFO_, "Recording start acknowledged.");
    }
    else if(!recAkn && !recReqst && !isRecording())
    {
        recAkn = true;
        logger.recordLogData(mmfs::INFO_, "Recording stop acknowledged.");
    }
}