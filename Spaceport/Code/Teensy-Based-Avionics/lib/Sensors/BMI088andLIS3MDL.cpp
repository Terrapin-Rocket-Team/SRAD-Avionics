//
// Created by ramykaddouri on 9/24/24.
// Modified to combine with LIS3MDL magnetometer
//

#include "BMI088andLIS3MDL.h"
using namespace mmfs;

bool BMI088andLIS3MDL::init()
{

    Wire.begin();

    int accelStatus = accel.begin();
    int gyroStatus = gyro.begin();

    printf("acc: %d", accelStatus);
    printf("gyro: %d", gyroStatus);

    int magStatus = mag.init();
    if (magStatus != 0)
    {
        mag.enableDefault();
    }

    initialized = (accelStatus > 0 && gyroStatus > 0);
    return initialized;
}

void BMI088andLIS3MDL::read()
{
    accel.readSensor();
    gyro.readSensor();
    mag.read();

    measuredMag = mmfs::Vector<3>(mag.m.x, mag.m.y, mag.m.z);
    measuredAcc = mmfs::Vector<3>(accel.getAccelX_mss(), accel.getAccelY_mss(), accel.getAccelZ_mss());
    measuredGyro = mmfs::Vector<3>(gyro.getGyroX_rads(), gyro.getGyroY_rads(), gyro.getGyroZ_rads());
}

void BMI088andLIS3MDL::packData()
{
    float accX = float(measuredAcc.x());
    float accY = float(measuredAcc.y());
    float accZ = float(measuredAcc.z());
    float gyroX = float(measuredGyro.x());
    float gyroY = float(measuredGyro.y());
    float gyroZ = float(measuredGyro.z());
    float magX = float(measuredMag.x());
    float magY = float(measuredMag.y());
    float magZ = float(measuredMag.z());

    int offset = 0;
    memcpy(packedData + offset, &accX, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &accY, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &accZ, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &gyroX, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &gyroY, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &gyroZ, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &magX, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &magY, sizeof(float));
    offset += sizeof(float);
    memcpy(packedData + offset, &magZ, sizeof(float));
    
}
