#include <Adafruit_BNO055.h>
#include <Arduino.h>
#include "IMU.h"

virtual void calibrate();
virtual imu::Quaternion get_orientation();
virtual imu::Vector<3> get_acceleration();
virtual imu::Vector<3> get_orientation_euler();
virtual String getcsvHeader();
virtual String getdataString();


