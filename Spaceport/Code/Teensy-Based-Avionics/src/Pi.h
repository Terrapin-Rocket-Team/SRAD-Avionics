#ifndef PI_H
#define PI_H

#include <Arduino.h>
#include <BlinkBuzz/BlinkBuzz.h>
#include <RecordData/Logger.h>

class Pi
{
public:
    Pi(int pinCmd, int pinResp);
    void startRec();
    void stopRec();
    bool isRecording();
    void check();
    ~Pi() { stopRec(); pinMode(pinCmd, INPUT); pinMode(pinResp, INPUT); }

private:
    int pinCmd;
    int pinResp;
    bool recReqst;
    bool recAkn;
};

#endif // PI_H