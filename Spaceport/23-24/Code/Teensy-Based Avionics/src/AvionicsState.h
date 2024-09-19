#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

// Platformio is such a fucking pile of trash
#include "MMFS.h"

using namespace mmfs;
class AvionicsState : public State
{
public:
    AvionicsState(Sensor **sensors, int numSensors, Filter *filter, Logger *logger, bool stateRecordsOwnData = true);
    void updateState(double newTime = -1) override;

private:
    char stages[6][15] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight"};
    void determineStage();
    int stage;
    double timeOfLaunch;
    double timeOfLastStage;
    double timeOfDay;
};
#endif