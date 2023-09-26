// Placeholder file for the IMU class

#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

#ifndef __ADAFRUIT_BNO055_H__
#include <imumaths.h>
#endif

class IMU {
public:
    virtual ~IMU() {}; //Virtual descructor. Very important
    virtual void calibrate() = 0; //Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual imu::Quaternion get_orientation() = 0;
    virtual imu::Vector<3> get_acceleration() = 0;
    virtual imu::Vector<3> get_orientation_euler() = 0;
    virtual String getcsvHeader() = 0;
    virtual String getdataString() = 0;
};


#endif 