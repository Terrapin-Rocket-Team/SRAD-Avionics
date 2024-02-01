#include "MAX_M10S.h"

/*
Constructor for Max-M10s 
*/
MAX_M10S::MAX_M10S(uint8_t SCK, uint8_t SDA, uint8_t address) {
    SCK_pin = SCK;
    SDA_pin = SDA; 

    first_fix = false;
    origin.x() = 38.987202;
    origin.y() = -76.945999;
    origin.z() = 0;
    altitude = 0;
    pos.x() = 0;
    pos.y() = 0;
    velocity.x() = -1;
    velocity.y() = -1;
    velocity.z() = -1;
    displacement.x() = -1;
    displacement.y() = -1;
    displacement.z() = -1;
    gps_time = 0;
    irl_time.x() = -1;
    irl_time.y() = -1;
    irl_time.z() = -1;
    fix_qual = -1;
}

//need to update origin some how

void MAX_M10S::initialize() {
    // Serial.println("Max-M10s");

    Wire.begin(); // Start I2C

    //myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial
    delay(25); // Wait for the serial port to initialize
    while (m10s.begin() == false) //Connect to the u-blox module using Wire port
    {
        Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
        delay (1000);
    }

    m10s.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
    
    //myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Optional: save (only) the communications port settings to flash and BBR
}

/*
used to update all instance variables 
*/
void MAX_M10S::read_gps() {
    if (!first_fix && m10s.getPVT() == true) {
        first_fix = true;
        origin.x() = m10s.getLatitude() / 10000000.0;
        origin.y() = m10s.getLongitude() / 10000000.0;
        origin.z() = m10s.getAltitude() / 1000.0;
    }
    altitude = m10s.getAltitude() / 1000.0; 

    pos.x() = m10s.getLatitude() / 10000000.0;
    pos.y() = m10s.getLongitude() / 10000000.0;

    // updated before displacement and gps as the old values and new values are needed to get a 
    // significant of a velocity
    velocity.x() = ((pos.x() * 111139.0) - displacement.x()) / (millis() / 1000.0- gps_time);
    velocity.y() = ((pos.y() * 111139.0) - displacement.y()) / (millis() / 1000.0 - gps_time);
    velocity.z() = ((altitude / 1000.0) - displacement.z()) / (millis() / 1000.0 - gps_time); 

    displacement.x() = (m10s.getLatitude() / 10000000.0 - origin.x()) * 111139.0;
    displacement.y() = (m10s.getLongitude() / 10000000.0- origin.y()) * 111139.0;
    displacement.z() = ((m10s.getAltitude() / 1000.0) - origin.z());

    gps_time = (millis() / 1000.0);
    
    irl_time.x() = m10s.getHour();
    irl_time.y() = m10s.getMinute();
    irl_time.z() = m10s.getSecond();

    fix_qual = m10s.getSIV();
}

/*
return altitude in m
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
returns vector of orginal position in lat(deg), lat(deg), and alti(m)
*/
imu::Vector<3> MAX_M10S::get_origin_pos() {
    return origin;
}

bool MAX_M10S::get_first_fix() {
    return first_fix;
}

/*
time since in initialization in seconds
*/
double MAX_M10S::get_gps_time() {
    return gps_time;
}

/*
returns the current hour, min, and sec 
*/
imu::Vector<3> MAX_M10S::get_irl_time() {   
    return irl_time;
}

/*
return the number of satellites to indicate quality of data
*/
int MAX_M10S::get_fix_qual() {
    return fix_qual;
}   

String MAX_M10S::getcsvHeader() {
    return "Latitude (deg), Longitude (deg), "
        "Altitude (mm), "
        "Velocity (m/s), "
        "Displacement X (m), Displacement Y (m), Displacement Z (m), "
        "gps time (s), "
        "quality of data (satellites), "
        "real time (hr/min/s)";

}

String MAX_M10S::getdataString() {
    return String(pos.x()) + ", " + String(pos.y()) + ", " + 
        String(altitude) + ", " +
        String(sqrt(pow(velocity.x(), 2) + pow(velocity.y(), 2) + pow(velocity.z(), 2))) + ", " + 
        String(displacement.x()) + ", " + String(displacement.y()) + ", " + String(displacement.z()) + ", " + 
        String(gps_time) + ", " +
        String(fix_qual) + ", " + 
        String(irl_time.x()) + ":" + String(irl_time.y()) + ":" + String(irl_time.z());
    
}

String MAX_M10S::getStaticDataString() {
    return "Original Latitude (m): " + String(origin.x()) + "\n" +
        "Original Longitude (m): " + String(origin.y()) + "\n" +
        "Original Altitude (m): " +  String(origin.z()) + "\n";
}

// Danny S.