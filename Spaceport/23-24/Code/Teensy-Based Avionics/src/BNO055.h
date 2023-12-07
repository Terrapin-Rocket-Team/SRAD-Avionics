#include <Adafruit_BNO055.h>
#include <Arduino.h>
#include "IMU.h"


class BNO055: public IMU {
private:
imu::Vector<3> acceleration_vec;
imu::Vector<3> euler_vec;


public:
BNO055(); 
virtual void setup_bno();
virtual void calibrate_bno();
virtual imu::Quaternion get_orientation();
virtual imu::Vector<3> get_acceleration();
virtual imu::Vector<3> get_orientation_euler();
virtual String getcsvHeader();
virtual String getdataString();

};


