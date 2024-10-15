#ifndef LIS_H
#define LIS_H
#include "Sensors/IMU/IMU.h"


bool LIS::init(){
        int accelStatus = accel.begin();
        int gyroStatus = gyro.begin();

        initialized = (accelStatus > 0 && gyroStatus > 0);      // AND gate
        return initialized;
}

void LIS::read(){
        accel.readSensor();
        gyro.readSensor();

        measuredAcc = mmfs::Vector<3>(accel.getAccelX_mss(), accel.getAccelY_mss(), accel.getAccelZ_mss());
        measuredGyro = mmfs::Vector<3>(gyro.getGyroX_rads(), gyro.getGyroY_rads(), gyro.getGyroZ_rads());
        measuredMag = 
}




