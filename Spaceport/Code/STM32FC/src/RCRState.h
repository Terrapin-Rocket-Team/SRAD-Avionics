#ifndef RCR_STATE_H
#define RCR_STATE_H

#include <State/State.h>

class RCRState : public astra::State{

    public:
    using State::State;

    private: 
    void determineStage() {}
    
};

#endif