//
// Created by ramykaddouri on 9/24/24.
// Modified to combine with LIS3MDL magnetometer
//

#include "BMI088andLIS3MDL.h"

bool BMI088andLIS3MDL::init() {

    Wire.begin();

    int accelStatus = accel.begin();
    int gyroStatus = gyro.begin();

    int magStatus = mag.init();
    if (magStatus != 0) {
        mag.enableDefault();
    }

    initialized = (accelStatus > 0 && gyroStatus > 0);
    return initialized;
}

void BMI088andLIS3MDL::read() {
    accel.readSensor();
    gyro.readSensor();
    mag.read();


    measuredMag = mmfs::Vector<3>(mag.m.x, mag.m.y, mag.m.z);
    measuredAcc = mmfs::Vector<3>(accel.getAccelX_mss(), accel.getAccelY_mss(), accel.getAccelZ_mss());
    measuredGyro = mmfs::Vector<3>(gyro.getGyroX_rads(), gyro.getGyroY_rads(), gyro.getGyroZ_rads());
}


