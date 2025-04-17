#ifndef PI_H
#define PI_H

#include <Arduino.h>
#include <BlinkBuzz/BlinkBuzz.h>

class Pi
{
public:
    Pi(int pinControl, int pinVideo);
    // void setOn(bool on);
    void setRecording(bool recording);
    // bool isOn();
    bool isRecording();

private:
    int pinControl;
    int pinVideo;
    bool on;
    bool recording;
};

#endif // PI_H