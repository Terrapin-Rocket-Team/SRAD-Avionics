#include "Pi.h"

Pi::Pi(int pinControl, int pinVideo)
{
    this->pinControl = pinControl;
    this->pinVideo = pinVideo;

    pinMode(pinControl, OUTPUT);
    pinMode(pinVideo, OUTPUT);

    digitalWrite(pinControl, LOW); // Set control pin to low by default
    digitalWrite(pinVideo, LOW);   // Set video pin to low by default

    on = false;
    recording = false;
}

void Pi::setOn(bool on)
{
    if (this->on == on)
        return;
    // pulse high to toggle
    digitalWrite(this->pinControl, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->pinControl, LOW);
    bb.aonoff(mmfs::BUZZER, 50, 5); // Buzz 3 times (100ms on, 100ms off)

    this->on = on;
}

void Pi::setRecording(bool recording)
{
    if (this->recording == recording)
        return; // If the recording state is the same, do nothing

    // pulse high to toggle
    digitalWrite(this->pinVideo, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->pinVideo, LOW);
    bb.aonoff(mmfs::BUZZER, 50, 2); // Buzz 3 times (100ms on, 100ms off)
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