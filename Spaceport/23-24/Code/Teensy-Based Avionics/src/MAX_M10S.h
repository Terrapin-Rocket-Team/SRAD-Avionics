#ifndef MAX_M10S_H
#define MAX_M10S_H

#include <Wire.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <Arduino.h>
#include "GPS.h"

class MAX_M10S : public GPS {
private:
    SFE_UBLOX_GNSS m10s;
    uint8_t SCK_pin;
    uint8_t SDA_pin;
    u_int8_t i2c_address;
    imu::Vector<2> pos;             // latitude and longitude
    double altitude;                // alti in mm 
    imu::Vector<3> velocity;        // m per s
    imu::Vector<3> displacement;    // m from starting location
    imu::Vector<3> origin;          // lat(deg), long(deg), alti(m) of the original location
    double gps_time;                // time since start of program in seconds
    int fix_qual;                   // num of connections to satellites
    imu::Vector<3> irl_time;        // returns the current hour, min, and sec 
    bool first_fix;                 // whether or not gps has recieved first fix

public:
    MAX_M10S(uint8_t SCK, uint8_t SDA, uint8_t address);
    void read_gps();
    double get_alt();
    imu::Vector<3> get_velocity();
    imu::Vector<2> get_pos();
    imu::Vector<3> get_displace();
    double get_gps_time();
    bool get_first_fix();
    imu::Vector<3> get_irl_time();
    imu::Vector<3> get_origin_pos();
    int get_fix_qual();
    void *get_data();
    bool initialize() override;
    const char *getcsvHeader() override;
    char *getdataString() override;
    char *getStaticDataString() override;
};

#endif //MAX_M10S_H


// Danny S.