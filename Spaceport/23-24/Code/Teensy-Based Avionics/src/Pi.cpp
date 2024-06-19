#include "Pi.h"

Pi::Pi(int pinControl, int pinVideo)
{
    this->pinControl = pinControl;
    this->pinVideo = pinVideo;

    pinMode(pinControl, OUTPUT);
    pinMode(pinVideo, OUTPUT);

    digitalWrite(pinVideo, LOW); // Set video pin to high (off) by default

    on = false;
    recording = false;
}

void Pi::setOn(bool on)
{
    digitalWrite(this->pinControl, on ? HIGH : LOW);
    this->on = on;
}

void Pi::setRecording(bool recording)
{
    if (this->recording == recording)
        return; // If the recording state is the same, do nothing

    if (recording)
        bb.aonoff(BUZZER, 500, 2, 100); // Buzz twice for recording state change
    digitalWrite(pinVideo, recording ? HIGH : LOW);
    this->recording = recording;
}

bool Pi::isOn()
{
    return on;
}

bool Pi::isRecording()
{
    return recording;
}