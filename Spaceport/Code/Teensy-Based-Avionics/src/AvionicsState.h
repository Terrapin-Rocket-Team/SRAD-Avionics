#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

// Platformio is such a fucking pile of trash
#include "MMFS.h"

using namespace mmfs;
class AvionicsState : public State
{
public:
    AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter);
    void updateState(double newTime = -1) override;
    uint32_t getStage() { return stage; }

private:
    char stages[6][15] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight"};
    void determineStage();
    uint32_t stage;
    double timeOfLaunch;
    double timeOfLastStage;
    double timeOfDay;
};
#endif