#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

// Platformio is such a fucking pile of trash
#include <State/State.h>
#include <BlinkBuzz/BlinkBuzz.h>

    class AvionicsState : public mmfs::State
    {
    public:
        AvionicsState(Sensor **sensors, int numSensors, mmfs::KalmanInterface *kfilter, bool recordData);
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