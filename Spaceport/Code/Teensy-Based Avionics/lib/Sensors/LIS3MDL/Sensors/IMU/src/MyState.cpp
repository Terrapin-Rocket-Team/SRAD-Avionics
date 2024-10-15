#include "MyState.h"

using namespace mmfs;

MyState::MyState(Sensor **sensors, int numSensors, Filter *filter, Logger *logger, bool stateRecordsOwnData = true)
    : State(sensors, numSensors, filter, logger, stateRecordsOwnData)
{
    // initialize your state variables here
    lastTime = 0;
    currentTime = 0;
}

void MyState::updateState(double newTime)
{
    // update your state in this method
    lastTime = currentTime;
    if (newTime != -1)
        currentTime = newTime;
    else
        currentTime = millis() / 1000.0;

    updateSensors();
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));

    if (sensorOK(baro))
    {

        //-------------------Implement Code Here-------------------//

        // read your barometer's raw data here

        // pass the barometer reading into the FIR filter and call the filter's update/iterate method

        // get the filtered data from the filter and update your state variables
        // if needed, use del
    }

    setDataString();
    if (recordOwnFlightData)
        logger->recordFlightData(dataString);
}