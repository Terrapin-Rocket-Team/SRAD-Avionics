#ifndef PI_H
#define PI_H

#include <Arduino.h>
#include <BlinkBuzz/BlinkBuzz.h>

class Pi
{
public:
    Pi(int pinControl, int pinVideo);
    void startRec();
    void stopRec();
    bool isOn();
    bool isRecording();
    void check();

private:
    int pinCmd;
    int pinResp;
    bool recReqst, recAkn;
};

#endif // PI_H