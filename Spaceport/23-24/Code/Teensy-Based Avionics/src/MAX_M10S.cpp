#include "MAX_M10S.h"

/*
Constructor for Max-M10s 
*/
MAX_M10S::MAX_M10S() {
    orgin.x() = m10s.getLatitude();
    orgin.y() = m10s.getLongitude();
    orgin.z() = m10s.getAltitude() * 1000;
    read_gps();
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
    velocity.x() = ((m10s.getLatitude() * 111139.0) - displacement.x()) / (millis() - gps_time);
    velocity.y() = ((m10s.getLongitude() * 111139.0) - displacement.y()) / (millis() - gps_time);
    velocity.z() = ((m10s.getAltitude() * 1000.0) - displacement.z()) / (millis() - gps_time);
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
    displacement.x() = (m10s.getLatitude() - orgin.x()) * 111139.0;
    displacement.y() = (m10s.getLongitude() - orgin.y()) * 111139.0;
    displacement.z() = ((m10s.getAltitude() * 1000) - orgin.z());
    return displacement;
}

/*
time since in initialization in seconds
*/
double MAX_M10S::get_gps_time() {
    return gps_time = (millis() * 1000);
}
/*
returns the current hour, min, and sec 
*/
imu::Vector<3> MAX_M10S::get_time() {
    time.x() = m10s.getHour();
    time.y() = m10s.getMinute();
    time.z() = m10s.getSecond();
    return time;
}

/*
return the number of satellites to indicate quality of data
*/
int MAX_M10S::get_fix_qual() {
    return fix_qual = m10s.getSIV();
}   
