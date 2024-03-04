// Placeholder file for the IMU class

#ifndef IMU_H
#define IMU_H

#include "Sensor.h"

#ifndef __ADAFRUIT_BNO055_H__
#include <imumaths.h>
#endif

class IMU : public Sensor{
public:
    virtual ~IMU() {}; //Virtual descructor. Very important
    virtual imu::Quaternion getOrientation() = 0;
    virtual imu::Vector<3> getAcceleration() = 0;
    virtual imu::Vector<3> getOrientationEuler() = 0;
    virtual imu::Vector<3> getMagnetometer() = 0;
    virtual SensorType getType() override { return IMU_; }
    virtual const char* getTypeString() override { return "IMU"; }
};


#endif 