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

void Pi::setOn(bool on)
{
    if (this->on == on)
        return;
    digitalWrite(this->pinControl, LOW);
    delayMicroseconds(10);
    digitalWrite(this->pinControl, HIGH);
    bb.aonoff(mmfs::BUZZER, 50, 5); // Buzz 3 times (100ms on, 100ms off)

    this->on = on;
}

void Pi::setRecording(bool recording)
{
    if (this->recording == recording)
        return; // If the recording state is the same, do nothing

    bb.aonoff(mmfs::BUZZER, 50, 2); // Buzz 3 times (100ms on, 100ms off)
    digitalWrite(this->pinVideo, LOW);
    delayMicroseconds(10);
    digitalWrite(this->pinVideo, HIGH);
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