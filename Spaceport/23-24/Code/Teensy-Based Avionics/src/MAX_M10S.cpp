#include "MAX_M10S.h"

/*
Constructor for Max-M10s
*/
MAX_M10S::MAX_M10S(uint8_t SCK, uint8_t SDA, uint8_t address)
{
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

// need to update origin some how

bool MAX_M10S::initialize()
{

    // myGNSS.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

    int count = 0;
    while (m10s.begin() == false && count < 3) // Connect to the u-blox module using Wire port
    {
        // Serial.println(F("u-blox GNSS not detected at default I2C address. Retrying..."));
        delay(1000);
        count++;
    }
    if (!m10s.begin())
        return false;

    m10s.setI2COutput(COM_TYPE_UBX);            // Set the I2C port to output UBX only (turn off NMEA noise)
    m10s.setNavigationFrequency(10);            // Set the update rate to 10Hz
    m10s.setDynamicModel(DYN_MODEL_AIRBORNE4g); // Set the dynamic model to airborne with 4g acceleration
    m10s.setAutoPVT(true);                      // Enable automatic PVT reports
    m10s.saveConfiguration();                   // Save the current settings to flash and BBR
    return true;
}

/*
used to update all instance variables
*/
void MAX_M10S::read_gps()
{
    if (!m10s.getPVT() || m10s.getInvalidLlh())
        return; // See if new data is available

    double time = millis() / 1000.0;
    pos.x() = m10s.getLatitude() / 10000000.0;
    pos.y() = m10s.getLongitude() / 10000000.0;
    altitude = m10s.getAltitude() / 1000.0;

    if (!first_fix)
    {
        //recordLogData(INFO, "GPS has first fix"); //Log this data when the new data logging branch is merged.
        first_fix = true;
        origin.x() = pos.x();
        origin.y() = pos.y();
        origin.z() = altitude;
    }

    // updated before displacement and gps as the old values and new values are needed to get a
    // significant of a velocity
    velocity.x() = ((pos.x() * 111139.0) - displacement.x()) / (time - gps_time);
    velocity.y() = ((pos.y() * 111139.0) - displacement.y()) / (time - gps_time);
    velocity.z() = ((altitude)-displacement.z()) / (time - gps_time);

    displacement.x() = (pos.x() - origin.x()) * 111139.0;
    displacement.y() = (pos.y() - origin.y()) * 111139.0;
    displacement.z() = (altitude - origin.z());

    gps_time = time;

    fix_qual = m10s.getSIV();
}

/*
return altitude in m
*/
double MAX_M10S::get_alt()
{
    return altitude;
}

/*
returns the lat and long of the rocket to the 7th sig fig
*/
imu::Vector<2> MAX_M10S::get_pos()
{
    return pos;
}

/*
return the velocity (meters per second)
there probably issues with floating points
*/
imu::Vector<3> MAX_M10S::get_velocity()
{
    return velocity;
}

/*
retern the displacement since the origin
there is probably issues with floating point arithmetic
*/
imu::Vector<3> MAX_M10S::get_displace()
{
    return displacement;
}

/*
returns vector of orginal position in lat(deg), lat(deg), and alti(m)
*/
imu::Vector<3> MAX_M10S::get_origin_pos()
{
    return origin;
}

bool MAX_M10S::get_first_fix()
{
    return first_fix;
}

/*
time since in initialization in seconds
*/
double MAX_M10S::get_gps_time()
{
    return gps_time;
}

/*
return the number of satellites to indicate quality of data
*/
int MAX_M10S::get_fix_qual()
{
    return fix_qual;
}

void *MAX_M10S::get_data()
{
    return (void *)&pos;
}

const char *MAX_M10S::getcsvHeader()
{                                                                                                                         // incl G- for GPS
    return "G-Lat (deg),G-Lon (deg),G-Alt (m),G-Speed (m/s),G-DispX (m),G-DispY (m),G-DispZ (m),G-Time (s),G-# of Sats,"; // trailing comma
}
char *MAX_M10S::getdataString()
{
    // See State.cpp::setdataString() for comments on what these numbers mean. 15 for GPS.
    const int size = 15 * 2 + 12 * 5 + 10 * 1 + 10;
    char *data = new char[size];
    snprintf(data, size, "%.10f,%.10f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,", pos.x(), pos.y(), altitude, sqrt(pow(velocity.x(), 2) + pow(velocity.y(), 2) + pow(velocity.z(), 2)), displacement.x(), displacement.y(), displacement.z(), gps_time, fix_qual); // trailing comma
    return data;
}
char *MAX_M10S::getStaticDataString()
{
    // See State.cpp::setdataString() for comments on what these numbers mean. 15 for GPS.
    const int size = 60 + 15 * 2 + 12 * 1;
    char *data = new char[size];
    snprintf(data, size, "Original Latitude (m): %.10f\nOriginal Longitude (m): %.10f\nOriginal Altitude (m): %.2f\n", origin.x(), origin.y(), origin.z());
    return data;
}

// Danny S.