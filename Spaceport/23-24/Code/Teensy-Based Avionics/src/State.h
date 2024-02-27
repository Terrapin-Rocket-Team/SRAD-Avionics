#ifndef STATE_H
#define STATE_H

// Include all the sensor classes
#include "Barometer.h"
#include "GPS.h"
#include "IMU.h"
#include "LightSensor.h"
#include "Radio.h"
#include "RTC.h"
#include "RecordData.h"

const char STAGES[][15] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight"};
class State
{
public:
    double timeAbsolute;         // in s
    imu::Vector<3> position;     // in m from start position
    imu::Vector<3> velocity;     // in m/s
    imu::Vector<3> acceleration; // in m/s^2
    imu::Quaternion orientation; // in quaternion

    State();
    ~State();
    // to be called after all applicable sensors have been added.
    // Retruns false if any sensor failed to init. check for getSensor == nullptr to see which sensor failed. Disables sensor if failed.
    bool init();
    void settimeAbsolute();
    void updateState();
    int getStageNum();


    // add sensor functions
    void setBaro(Barometer *Barometer);
    void setGPS(GPS *gps);
    void setIMU(IMU *imu);
    void setLS(LightSensor *LightSensor);
    void setRTC(RTC *rtc);

    Barometer *getBaro();
    GPS *getGPS();
    IMU *getIMU();
    LightSensor *getLS();
    RTC *getRTC();

    char *getdataString();
    char *getcsvHeader();
    char *getStateString(); // This contains only the portions that define what the state thinks the rocket looks like. I recommend sending this over the radio during launches.

    double apogee;                 // in m above start position
    double accelerationMagnitude;  // in m/s^2
    double timeLaunch;             // in ms
    double timeSinceLaunch;        // in ms
    double timePreviousStage;      // in ms
    double timeSincePreviousStage; // in ms

    void determinetimeSincePreviousStage();
    void determinetimeSinceLaunch();
    void determineaccelerationMagnitude(imu::Vector<3> accel);
    void determineapogee(double zPosition);

private:
    char *dataString;
    char *stateString;
    char *csvHeader;
    int stageNumber;
    int lastGPSUpdate;
    int numSensors;
    Barometer *baro;
    GPS *gps;
    IMU *imu;
    LightSensor *lisens;
    RTC *rtc;
    int landingCounter;//used to save a bit of data after landing
    void setcsvHeader();
    void setdataString();
    void updateSensors();
    void updateStage(int num);//updates the recordData stage and the stageNumber
};

#endif