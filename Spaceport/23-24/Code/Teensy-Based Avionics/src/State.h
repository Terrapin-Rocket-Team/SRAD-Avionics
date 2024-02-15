#ifndef STATE_H
#define STATE_H

// Include all the sensor classes
#include "Barometer.h"
#include "GPS.h"
#include "IMU.h"
#include "LightSensor.h"
#include "Radio.h"
#include "RTC.h"
#include <vector>
#include <numeric>

class State
{
public:
    double timeAbsolute;         // in s
    imu::Vector<3> position;     // in m from start position
    imu::Vector<3> velocity;     // in m/s
    imu::Vector<3> acceleration; // in m/s^2
    imu::Quaternion orientation; // in quaternion
    String stage;
    Barometer *stateBarometer;
    GPS *stateGPS;
    IMU *stateIMU;
    LightSensor *stateLightSensor;
    RTC *stateRTC;
    String csvHeader;
    int stageNumber;
    int lastGPSUpdate;

    State(); // constructor
    void setup();
    void settimeAbsolute();
    void setcsvHeader();
    void setdataString();
    void updateSensors();
    void updateState();
    String getdataString();
    String getrecordDataState();

    // add sensor functions
    void addBarometer(Barometer *Barometer);
    void addGPS(GPS *gps);
    void addIMU(IMU *imu);
    void addLightSensor(LightSensor *LightSensor);
    void addRTC(RTC *rtc);

    double apogee;                 // in m above start position
    double accelerationMagnitude;  // in m/s^2
    double timeLaunch;             // in ms
    double timeSinceLaunch;        // in ms
    double timePreviousStage;      // in ms
    double timeSincePreviousStage; // in ms

    bool barometerFlag;
    bool gpsFlag;
    bool imuFlag;
    bool lightSensorFlag;
    bool rtcFlag;

    String recordDataStage;

    void determinetimeSincePreviousStage();
    void determinetimeSinceLaunch();
    void determineaccelerationMagnitude(imu::Vector<3> accel);
    void determineapogee(double zPosition);

private:
    String dataString;
};

#endif