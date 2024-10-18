//
// Created by ramykaddouri on 9/24/24.
// Modified to combine with LIS3MDL magnetometer
//

#include "BMI088andLIS3MDL.h"

bool BMI088andLIS3MDL::init() {
    int accelStatus = accel.begin();
    int gyroStatus = gyro.begin();

    initialized = (accelStatus > 0 && gyroStatus > 0);
    return initialized;
}

void BMI088andLIS3MDL::read() {
    accel.readSensor();
    gyro.readSensor();

    measuredAcc = mmfs::Vector<3>(accel.getAccelX_mss(), accel.getAccelY_mss(), accel.getAccelZ_mss());
    measuredGyro = mmfs::Vector<3>(gyro.getGyroX_rads(), gyro.getGyroY_rads(), gyro.getGyroZ_rads());
}


