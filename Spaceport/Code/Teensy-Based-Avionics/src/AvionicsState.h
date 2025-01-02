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

    virtual const PackedType *getPackedOrder() const override;
    virtual const int getNumPackedDataPoints() const override;
    virtual const char **getPackedDataLabels() const override;

private:
    virtual void packData() override;
    char stages[7][20] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight", "Dumped"};
    void determineStage();
    uint32_t stage;
    double timeOfLaunch;
    double timeOfLastStage;
    double timeOfDay;
};
#endif