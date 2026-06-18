#include "Timer.h"

Timer::Timer(uint32_t interval, bool repeat) : interval(interval), repeat(repeat) {}

bool Timer::evaluate()
{
    // check if millis has wrapped
    if (millis() < this->internalTimer)
    {
        if (!this->done && this->interval >= 0 && millis() + (0xffffffff - this->internalTimer) > this->interval)
        {
            // reset the timer
            if (this->repeat)
                this->internalTimer = millis();
            else
                this->done = true;
            return true;
        }
        return false;
    }
    // if the length is positive and the given time interval has passed, the timer has gone off
    if (!this->done && this->interval >= 0 && millis() - this->internalTimer > this->interval)
    {
        // reset the timer
        if (this->repeat)
            this->internalTimer = millis();
        else
            this->done = true;
        return true;
    }
    return false;
}

void Timer::reset()
{
    this->internalTimer = millis();
    this->done = false;
}

uint32_t Timer::timeLeft()
{
    return this->interval - (millis() - this->internalTimer);
}