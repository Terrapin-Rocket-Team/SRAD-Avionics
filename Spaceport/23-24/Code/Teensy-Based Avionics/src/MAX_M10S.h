#ifndef MAX_M10S_H
#define MAX_M10S_H

#include <Wire.h>
#include <SparkFun_u-blox_GNSS_v3.h>
#include <Arduino.h>
#include "GPS.h"

class MAX_M10S : public GPS
{
private:
    SFE_UBLOX_GNSS m10s;
    uint8_t SCK_pin;
    uint8_t SDA_pin;
    uint8_t i2c_address;
    imu::Vector<2> pos;          // latitude and longitude
    double altitude;             // alti in mm
    imu::Vector<3> velocity;     // m per s
    imu::Vector<3> displacement; // m from starting location
    imu::Vector<3> origin;       // lat(deg), long(deg), alti(m) of the original location
    int fix_qual;                // num of connections to satellites
    imu::Vector<3> irl_time;     // returns the current hour, min, and sec
    bool first_fix;              // whether or not gps has recieved first fix
    double heading;
    int hr, min, sec;
    char gps_time[9];
    double time;

public:
    MAX_M10S(uint8_t SCK, uint8_t SDA, uint8_t address);
    double get_alt();
    imu::Vector<3> get_velocity();
    imu::Vector<2> get_pos();
    imu::Vector<3> get_displace();
    char *get_time_of_day();
    bool get_first_fix();
    imu::Vector<3> get_origin_pos();
    int get_fix_qual();
    double get_heading();
    void *getData();

    bool initialize() override;
    const char *getCsvHeader() override;
    char *getDataString() override;
    char *getStaticDataString() override;
    const char *getName() override;
    void update() override;
};

#endif // MAX_M10S_H

// Danny S.