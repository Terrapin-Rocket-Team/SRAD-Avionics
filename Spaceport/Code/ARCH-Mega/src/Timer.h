#ifndef TIMER_H
#define TIMER_H

#include "Arduino.h"

#define TIMER_REPEAT true
#define TIMER_ONCE false

// A class representing a Timer.
class Timer
{
public:
    // The interval the timer should go off at
    uint32_t interval = 0;
    // Internal variable used to keep track of the last time the timer went off
    uint32_t internalTimer = millis();
    // Whether the timer should automatically repeat
    bool repeat = TIMER_REPEAT;
    // Whether the timer has completed
    bool done = false;

    // Timer constructor
    Timer(uint32_t interval, bool repeat = TIMER_REPEAT);

    // check if the timer has gone off
    bool evaluate();

    // reset the timer to 0
    void reset();

    uint32_t timeLeft();
};

#endif