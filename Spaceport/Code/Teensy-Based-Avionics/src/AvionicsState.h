#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

// Platformio is such a fucking pile of trash
#include "MMFS.h"
#include "LKF.h"
#include "Mahony.h"

using namespace mmfs;
class AvionicsState : public State
{
public:
    AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter);
    void updateVariables() override;
    double getTimeSinceLastStage();
    bool init() override;

private:
    char stages[7][20] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight", "Dumped"};
    void determineStage() override;
    double timeOfLaunch;
    double timeOfLastStage;
    Quaternion orientationFromInitial;

    MahonyAHRS ahrs;
    unsigned long lastMicros = 0;

    LKF lkf;
    CircBuffer<Vector<4>> buf;
};

    static int iter = 0;
#endif