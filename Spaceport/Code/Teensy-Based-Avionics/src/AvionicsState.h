#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

// Platformio is such a fucking pile of trash
#include "MMFS.h"

using namespace mmfs;
class AvionicsState : public State
{
public:
    AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter);
    void updateVariables() override;
    double getTimeSinceLastStage();

private:
    char stages[7][20] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight", "Dumped"};
    void determineStage() override;
    double timeOfLaunch;
    double timeOfLastStage;
    double imuVelocity;
    int consecutiveNegativeBaroVelocity;
};
#endif