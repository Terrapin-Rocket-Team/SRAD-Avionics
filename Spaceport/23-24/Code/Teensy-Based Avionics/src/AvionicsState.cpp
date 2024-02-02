#ifndef AVIONICSSTATE_H
#define AVIONICSSTATE_H

#include <Arduino.h>
#include "State.h"

class AvionicsState : public State
{

    int stageNumber = 0;

    void updateSensors() {
        if (barometerFlag) {
            stateBarometer->get_data();
        }
        if (imuFlag) {
            stateIMU->get_data();
            stateIMU->get_orientation();
            stateIMU->get_orientation_euler();
        }
    }

    void updateState() {
        if(gpsFlag) {
            position = imu::Vector<3>(stateGPS->get_pos().x(), stateGPS->get_pos().y(), stateGPS->get_alt());
            velocity = stateGPS->get_velocity();
        }
        if (barometerFlag) {
            velocity.z() = (stateBarometer->get_rel_alt_ft() - position.z()) / (millis() - timeAbsolute);
            position.z() = stateBarometer->get_rel_alt_ft();
        }
        if (imuFlag)
        {
            acceleration = stateIMU->get_acceleration();
            orientation = stateIMU->get_orientation();
        }
        settimeAbsolute();

        if (stageNumber == 0 && acceleration.z() > 20 && position.z() > 75) {
            stageNumber = 1;
            timeLaunch = timeAbsolute;
            timePreviousStage = timeAbsolute;
            stage = "Ascent";
        }
        else if (stageNumber == 1 && acceleration.z() < 5) {
            stageNumber = 2;
            stage = "Coasting";
        }
        else if (stageNumber == 2 && velocity.z() < 0){
            stageNumber = 3;
            stage = "Drogue Descent";
        }
        else if (stageNumber == 3 && position.z() < 750) {
            stageNumber = 4;
            stage = "Main Descent";
        }
        else if (stageNumber == 4 && velocity.z() > -0.5 && accelerationMagnitude < 5 && stateBarometer->get_rel_alt_ft() < 200) {
            stageNumber = 5;
            stage = "Landed";
        }

        // backup case to dump data (25 minutes)
        determinetimeSinceLaunch();
        if (timeSinceLaunch > 1500) {
            stageNumber = 5;
            stage = "Landed";
        }

        
        

    }



};

#endif 