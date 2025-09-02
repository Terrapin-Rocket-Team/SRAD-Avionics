#ifndef RCR_STATE_H
#define RCR_STATE_H

#include <State/State.h>

class RCRState : public mmfs::State{

    public:
    using State::State;

    void determineStage() override {}
    
};

#endif