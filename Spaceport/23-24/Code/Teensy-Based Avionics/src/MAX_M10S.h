#ifndef MAX_M10S_H
#define MAX_M10S_H

#include <Wire.h>
#include "SparkFun_u-blox_GNSS_v3.h"
#include <Arduino.h>
#include "GPS.h"

class MAX_M10S : public GPS {
private:
    double altitude;
    imu::Vector<3> velocity;
    imu::Vector<2> pos_lat_long;
    imu::Vector<3> displacement;
    int fix_qual;
    double gps_time;

public:
    MAX_M10S();
    void calibrate(); 
    void read_gps();
    double get_alt();
    imu::Vector<3> get_velocity();
    imu::Vector<2> get_pos();
    imu::Vector<3> get_displacement();
    double get_gps_time();
    int get_fix_qual();
    String getcsvHeader();
    String getdataString();
};

#endif //MAX_M10S_H
