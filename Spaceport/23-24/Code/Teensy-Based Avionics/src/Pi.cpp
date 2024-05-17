#include "Pi.h"

Pi::Pi(int pinControl, int pinVideo)
{
    this->pinControl = pinControl;
    this->pinVideo = pinVideo;
    pinMode(this->pinControl, OUTPUT);
    pinMode(this->pinVideo, OUTPUT);
    digitalWrite(this->pinVideo, HIGH); // Set video pin to high (off) by default
    this->on = false;
    this->recording = false;
}

void Pi::setOn(bool on)
{
    digitalWrite(this->pinControl, on ? HIGH : LOW);
    this->on = on;
}

void Pi::setRecording(bool recording)
{
    if(this->recording == recording) return; // If the recording state is the same, do nothing

    bb.aonoff(BUZZER, 100, 3); // Buzz 3 times (100ms on, 100ms off)
    digitalWrite(this->pinVideo, recording ? LOW : HIGH);
    this->recording = recording;
}

bool Pi::isOn()
{
    return this->on;
}

bool Pi::isRecording()
{
    return this->recording;
}