#include "MAX_M10S.h"

/*
Constructor for Max-M10s 
*/
MAX_M10S::MAX_M10S() {

}

//need to update origin some how

void MAX_M10S::calibrate() {
    Serial.println("Max-M10s");
}

/*
used to update all instance variables 
*/
void MAX_M10S::read_gps() {
    altitude = m10s.getAltitude();

    pos.x() = m10s.getLatitude();
    pos.y() = m10s.getLongitude();

    velocity.x() = ((pos.x() * 111139) - displacement.x()) / (millis() * 1000 - gps_time);
    velocity.y() = ((pos.y() * 111139) - displacement.y()) / (millis() * 1000 - gps_time);
    velocity.z() = ((altitude * 1000) - displacement.z()) / (millis() * 1000 - gps_time); 

    displacement.x() = (m10s.getLatitude() - orgin.x()) * 111139.0;
    displacement.y() = (m10s.getLongitude() - orgin.y()) * 111139.0;
    displacement.z() = ((m10s.getAltitude() * 1000) - orgin.z());

    gps_time = (millis() * 1000);

    real_time.x() = m10s.getHour();
    real_time.y() = m10s.getMinute();
    real_time.z() = m10s.getSecond();

    fix_qual = m10s.getSIV();
}

/*
return altitude in mm
*/
double MAX_M10S::get_alt() {
    return altitude;
}

/*
returns the lat and long of the rocket to the 7th sig fig 
*/
imu::Vector<2> MAX_M10S::get_pos() {
    return pos;
} 

/*
return the velocity (meters per second)
there probably issues with floating points
*/
imu::Vector<3> MAX_M10S::get_velocity() {
    return velocity;
}

/*
retern the displacement since the origin
there is probably issues with floating point arithmetic 
*/
imu::Vector<3> MAX_M10S::get_displace() {
    return displacement;
}

/*
time since in initialization in seconds
*/
double MAX_M10S::get_gps_time() {
    return gps_time ;
}

/*
returns the current hour, min, and sec 
*/
imu::Vector<3> MAX_M10S::get_time() {   
    return real_time;
}

/*
return the number of satellites to indicate quality of data
*/
int MAX_M10S::get_fix_qual() {
    return fix_qual;
}   

String getcsvHeader() {
    

}

String getdataString() {


}

// Danny S.