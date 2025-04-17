#include "Pi.h"

Pi::Pi(int pinControl, int pinVideo)
{
    this->pinControl = pinControl;
    this->pinVideo = pinVideo;

    pinMode(pinControl, INPUT);
    pinMode(pinVideo, OUTPUT);

    digitalWrite(pinVideo, HIGH); // Set video pin to high (off) by default

    on = false;
    recording = false;
}

// void Pi::setOn(bool on)
// {
//     digitalWrite(this->pinControl, on ? HIGH : LOW);
//     this->on = on;
// }

void Pi::setRecording(bool recording)
{
    if(this->recording == recording) return; // If the recording state is the same, do nothing

    // bb.aonoff(mmfs::BUZZER, 100, 3); // Buzz 3 times (100ms on, 100ms off)
    digitalWrite(pinVideo, recording ? LOW : HIGH);
    this->recording = recording;
}

// bool Pi::isOn()
// {
//     return on;
// }

bool Pi::isRecording()
{
    return recording;
}