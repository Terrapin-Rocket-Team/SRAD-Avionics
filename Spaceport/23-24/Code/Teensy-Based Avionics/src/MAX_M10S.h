#ifndef MAX_M10S_H
#define MAX_M10S_H

#include <Wire.h>
#include "SparkFun_u-blox_GNSS_v3.h"
#include <Arduino.h>
#include "GPS.h"

class MAX_M10S : public GPS {
private:
    SFE_UBLOX_GNSS m10s;
    double altitude;                // alti in mm 
    imu::Vector<3> velocity;        // meters per second
    imu::Vector<2> pos;             // latitude and longitude
    imu::Vector<3> displacement;    // meters from starting location
    imu::Vector<3> orgin;           // lat, long, alti of the original location
    double gps_time;                // time since start of program in seconds
    int fix_qual;                   // num of connections to satellites
    imu::Vector<3> time;            // returns the current hour, min, and sec 
    // mean sea level
public:
    MAX_M10S();
    void calibrate(); 
    void read_gps();
    double get_alt();
    imu::Vector<3> get_velocity();
    imu::Vector<2> get_pos();
    imu::Vector<3> get_displace();
    double get_gps_time();
    imu::Vector<3> get_time();
    int get_fix_qual();
    String getcsvHeader();
    String getdataString();
};

#endif //MAX_M10S_H
