
#include "AvionicsState.h"

using namespace mmfs;

AvionicsState::AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter) : State(sensors, numSensors, kfilter)
{
    stage = 0;
    timeOfLaunch = 0;
    timeOfLastStage = 0;
    timeOfDay = 0;
}

void AvionicsState::updateState(double newTime)
{
    State::updateState(newTime); // call base version for sensor updates
    determineStage(); // determine the stage of the flight
}

void AvionicsState::determineStage()
{
    int timeSinceLaunch = currentTime - timeOfLaunch;
    int timeSinceLastStage = currentTime - timeOfLastStage;
    IMU *imu = reinterpret_cast<IMU *>(getSensor(IMU_));
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));
    //GPS *gps = reinterpret_cast<GPS *>(getSensor(GPS_));
    if (stage == 0 &&
        (((sensorOK(imu) || sensorOK(baro)) &&
        (sensorOK(imu) ? abs(imu->getAccelerationGlobal().z()) > 25 : true) &&
        (sensorOK(baro) ? baro->getAGLAltFt() > 60 : true))
        || currentTime > 25))
    // if we are in preflight AND
    // we have either the IMU OR the barometer AND
    // imu is ok AND the z acceleration is greater than 29 ft/s^2 OR imu is not ok AND
    // barometer is ok AND the relative altitude is greater than 30 ft OR baro is not ok

    // essentially, if we have either sensor and they meet launch threshold, launch. Otherwise, it will never detect a launch.
    {
        bb.aonoff(33, 200);
        // logger.setRecordMode(FLIGHT);
        stage = 1;
        timeOfLaunch = currentTime;
        timeOfLastStage = currentTime;
        logger.recordLogData(INFO_, "Launch detected.");
        logger.recordLogData(INFO_, "Printing static data.");
        for (int i = 0; i < maxNumSensors; i++)
        {
            if (sensorOK(sensors[i]))
            {
                char logData[200];
                //snprintf(logData, 200, "%s: %s", sensors[i]->getName(), sensors[i]->getStaticDataString());
                logger.recordLogData(INFO_, logData);
                sensors[i]->setBiasCorrectionMode(false);
            }
        }
    } // TODO: Add checks for each sensor being ok and decide what to do if they aren't.
    else if (stage == 1 && abs(acceleration.z()) < 10 && timeSinceLastStage > 5)
    {
        bb.aonoff(33, 200, 2);
        timeOfLastStage = currentTime;
        stage = 2;
        logger.recordLogData(INFO_, "Coasting detected.");
    }
    else if (stage == 2 && baroVelocity <= 0 && timeSinceLastStage > 5)
    {
        bb.aonoff(33, 200, 3);
        char logData[100];
        snprintf(logData, 100, "Apogee detected at %.2f m.", position.z());
        logger.recordLogData(INFO_, logData);
        timeOfLastStage = currentTime;
        stage = 3;
        logger.recordLogData(INFO_, "Drogue conditions detected.");
    }
    else if (stage == 3 && baro->getAGLAltFt() < 1000 && timeSinceLastStage > 5)
    {
        bb.aonoff(33, 200, 4);
        stage = 4;
        timeOfLastStage = currentTime;
        logger.recordLogData(INFO_, "Main parachute conditions detected.");
        printf("Alt: %f\n", baro->getAGLAltFt());
    }
    else if (stage == 4 && baroVelocity > -1 && baro->getAGLAltFt() < 66 && timeSinceLastStage > 5)
    {
        bb.aonoff(33, 200, 5);
        timeOfLastStage = currentTime;
        stage = 5;
        logger.recordLogData(INFO_, "Landing detected. Waiting for 5 seconds to dump data.");
    }
    else if (stage == 5 && timeSinceLastStage > 5)
    {
        logger.setRecordMode(GROUND);
        logger.recordLogData(INFO_, "Dumped data after landing.");
    }
}