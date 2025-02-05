#include "Pi.h"

Pi::Pi(int pinCmd, int pinResp)
{
    this->pinCmd = pinCmd;
    this->pinResp = pinResp;

    pinMode(pinCmd, OUTPUT);
    pinMode(pinResp, INPUT);

    digitalWrite(pinCmd, LOW); // Set video pin to high (off) by default

    recReqst = false; 
    recAkn = false;
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