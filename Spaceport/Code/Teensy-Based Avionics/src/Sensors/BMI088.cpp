//
// Created by ramykaddouri on 9/24/24.
//

#include "BMI088.h"

bool BMI088::init() {
    int accelStatus = accel.begin();
    int gyroStatus = gyro.begin();

    initialized = (accelStatus > 0 && gyroStatus > 0);
    return initialized;
}

void BMI088::read() {
    accel.readSensor();
    gyro.readSensor();

    measuredAcc = mmfs::Vector<3>(accel.getAccelX_mss(), accel.getAccelY_mss(), accel.getAccelZ_mss());
    measuredGyro = mmfs::Vector<3>(gyro.getGyroX_rads(), gyro.getGyroY_rads(), gyro.getGyroZ_rads());
}


