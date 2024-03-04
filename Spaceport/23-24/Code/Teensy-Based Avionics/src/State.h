#ifndef STATE_H
#define STATE_H

#include "AbhiKalmanFilter.h"

// Include all the sensor classes
#include "Barometer.h"
#include "GPS.h"
#include "IMU.h"
#include "LightSensor.h"
#include "Radio.h"
#include "RTC.h"
#include "RecordData.h"

constexpr char STAGES[][15] = {"Pre-Flight", "Boosting", "Coasting", "Drogue Descent", "Main Descent", "Post-Flight"};
class State
{
public:
    State(bool useKalmanFilter = true);
    ~State();

    // to be called after all applicable sensors have been added.
    // Returns false if any sensor failed to init. check for getSensor == nullptr to see which sensor failed. Disables sensor if failed.
    bool init(bool stateRecordsOwnData = true);
    void updateState();
    int getStageNum();
    // sensor functions
    bool addSensor(Sensor *sensor);
    Sensor *getSensor(SensorType type);


    // deprecated to improve flexibility and extendability |
    Barometer *getBaro();                               // |
    GPS *getGPS();                                      // |
    IMU *getIMU();                                      // |
    // deprecated to improve flexibility and extendability |
  
  
    Radio *getRadio();
    void setRadio(Radio *r);


    char *getDataString();
    char *getCsvHeader();
    char *getStateString(); // This contains only the portions that define what the state thinks the rocket looks like. I recommend sending this over the radio during launches.

    bool transmit();
    double timeAbsolute;           // in s since uC turned on

private:
    static constexpr int NUM_MAX_SENSORS = 5; // update with the max number of expected sensors
    Sensor *sensors[NUM_MAX_SENSORS];
    int numSensors; // how many sensors are actually enabled

    double heading_angle; // in degrees
    
    char *csvHeader;
    char *dataString;
    char *stateString;

    Radio *radio;

    void setCsvHeader();
    void setDataString();

    void updateSensors();

    // Sensor types
    Barometer **baro;
    GPS **gps;
    IMU **imu;

    // internal state variables
    int landingCounter;       // used to save a bit of data after landing
    bool recordOwnFlightData; // used to determine if the state should record its own data or leave it to other functions

    // Helper functions
    void determineAccelerationMagnitude();
    void determineStage();
    bool applySensorType(int index); // points the sensor variables to an appropriate sensors[] index.
    void setCsvString(char *str, const char* start, int startSize, bool header);

    // State variables
    double apogee; // in m above start position
    int stageNumber;
    double accelerationMagnitude;  // in m/s^2
    double timeOfLaunch;           // in s since uC turned on
    double timeSinceLaunch;        // in s
    double timePreviousStage;      // in s
    double timeSincePreviousStage; // in s
    imu::Vector<3> position;       // in m from start position
    imu::Vector<3> velocity;       // in m/s
    imu::Vector<3> acceleration;   // in m/s^2
    imu::Quaternion orientation;   // in quaternion
    double baroVelocity;           // in m/s
    double baroOldAltitude;        // in m

    char launchTimeOfDay[9];

        //Kalman Filter settings
    bool useKF;
    void initKF(bool useBaro, bool useGps, bool useImu);
    akf::KFState *kfilter;
    // time pos x y z vel x y z acc x y z
    double *predictions;
    // gps x y z barometer z
    double *measurements;
    // imu x y z
    double *inputs;
};

#endif

/*
    To add sensors: Include a local variable for the sensor in the state class and edit the applySensorType() function to point the local variable to the correct index in the sensors[] array.
    Then, edit what the state class does with the sensor data in the updateState() function.

    It's that easy!
    Don't forget to call addSensor() with your new sensor.

    NOTE: sensors are pointers to sensor pointers. This allows them to be linked to the sensors[] so that changes in there affect the sensor variables.
    It does make some slightly weird sysntax to use the sensors, but it's not too bad.
    Instead of if(gps) you use if(*gps) and instead of gps->getPos() you use (*gps)->getPos().

*/