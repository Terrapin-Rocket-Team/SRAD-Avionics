#include "MAX_M10S.h"

/*
Constructor for Max-M10s 
*/
MAX_M10S::MAX_M10S() {
    displacement.x() = 0;
    displacement.y() = 0;
    displacement.z() = 0;
}

void MAX_M10S::calibrate() {
    Serial.println("Max-M10s");
}

void MAX_M10S::read_gps() {
    get_velocity();
    get_alt();
    get_pos();
    get_displace();
    get_gps_time();
    get_fix_qual();
}

/*
return altitude in mm
*/
double MAX_M10S::get_alt() {
    return altitude = m10s.getAltitude();
}

/*
return the velocity (meters per second)
there probably issues with floating points
*/
imu::Vector<3> MAX_M10S::get_velocity() {
    velocity.x() = (m10s.getLatitude() - (displacement.x() * 111139.0 )) / (millis() - gps_time);
    velocity.y() = (m10s.getLatitude() - (displacement.y() * 111139.0 )) / (millis() - gps_time);
    velocity.z() = (m10s.getLatitude() - (displacement.z() * 111139.0 )) / (millis() - gps_time);
    return velocity;
}

/*
returns the lat and long of the rocket to the 7th sig fig 
*/
imu::Vector<2> MAX_M10S::get_pos() {
    pos.x() = m10s.getLatitude();
    pos.y() = m10s.getLongitude();
    return pos;
}

/*
retern the displacement since the origin
there is probably issues with floating point arithmetic 
*/
imu::Vector<3> MAX_M10S::get_displace() {
    displacement.x() = (m10s.getLatitude() + (displacement.x() / 111139.0 )) * 111139.0;
    displacement.y() = (m10s.getLongitude() + (displacement.y() / 111139.0 )) * 111139.0;
    displacement.z() = (m10s.getAltitude() + (displacement.z() / 111139.0 )) * 111139.0;
    return displacement;
}

/*
time since in initialization in seconds
*/
double MAX_M10S::get_gps_time() {
    return gps_time = (millis() * 1000);
}

/*
return the number of satellites to indicate quality of data
*/
int MAX_M10S::get_fix_qual() {
    return fix_qual = m10s.getSIV();
}   
