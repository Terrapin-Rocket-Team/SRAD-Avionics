#ifndef MYSTATE_H
#define MYSTATE_H

#include <State/State.h>
#include <BlinkBuzz/BlinkBuzz.h>
#include "FIRFilter.h"

namespace mmfs
{
    class MyState : public State
    {
    public:
        MyState(Sensor **sensors, int numSensors, Filter *filter, Logger *logger, bool stateRecordsOwnData = true);

        void updateState(double newTime = -1) override;     // override the updateState method, implement your own state update logic
    };
}

#endif // MYSTATE_H