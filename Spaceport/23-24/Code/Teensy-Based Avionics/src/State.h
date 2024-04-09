#ifndef STATE_H
#define STATE_H

#include "AprilFilter.h"
#include "Kalman_Filter.h"

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
    // useKalmanFilter: whether or not to use the Kalman Filter. If false, the state will use the raw sensor data.
    // stateRecordsOwnData: whether or not the state should call recordFlightData() itself. If false, other funcitons must call recordFlightData() to record the state's data.
    explicit State(bool useKalmanFilter = true, bool stateRecordsOwnData = true);
    ~State();

    // to be called after all applicable sensors have been added.
    // Returns false if any sensor failed to init. check data log for failed sensor. Disables sensor if failed.
    bool init();
    void settimeAbsolute();
    void updateState(double newTimeAbsolute = -1);
    int getStageNum();
    // sensor functions
    bool addSensor(Sensor *sensor, int sensorNum = 1);     // add more than one sensor of the same type, and specify which number this one is. 1 indexed. i.e. addSensor(gps, 2) adds the second GPS sensor.
    Sensor *getSensor(SensorType type, int sensorNum = 1); // get a sensor of a certain type. 1 indexed. i.e. getSensor(GPS, 1) gets the first GPS sensor.

    // deprecated to improve flexibility and extendability
    Barometer *getBaro(); // |
    GPS *getGPS();        // |
    IMU *getIMU();        // |
    // deprecated to improve flexibility and extendability

    Radio *getRadio();
    void setRadio(Radio *r);

    char *getDataString();
    char *getCsvHeader();
    char *getStateString(); // This contains only the portions that define what the state thinks the rocket looks like.

    bool transmit();
    double timeAbsolute; // in s since uC turned on

private:
    int lastTimeAbsolute;
    static constexpr int NUM_MAX_SENSORS = 3;                              // update with the max number of expected sensors.
    SensorType SENSOR_ORDER[NUM_MAX_SENSORS] = {BAROMETER_, GPS_, IMU_}; // make this array the same length as NUM_MAX_SENSORS and fill it.
    // example if you have more than one of the same sensor type:
    // constexpr SensorType SENSOR_ORDER[] = {BAROMETER_, BAROMETER_, GPS_, IMU_}; or
    // constexpr SensorType SENSOR_ORDER[] = {BAROMETER_, GPS_, IMU_, BAROMETER_}; It doesn't what order they're in, as long as they're in the array.
    const char SENSOR_NAMES[NUM_MAX_SENSORS][10] = {"Barometer", "GPS", "IMU" /*, "Barometer 2"*/}; // make this array the same length as NUM_MAX_SENSORS and fill it with the names of the sensors in the same order as SENSOR_ORDER

    Sensor *sensors[NUM_MAX_SENSORS];
    int numSensors; // how many sensors are actually enabled

    char *csvHeader;
    char *dataString;
    char *stateString;

    Radio *radio;

    void setCsvHeader();
    void setDataString();

    void updateSensors();

    // Sensor types
    Barometer *baro;
    GPS *gps;
    IMU *imu;

    // internal state variables
    int landingCounter;       // used to save a bit of data after landing
    bool recordOwnFlightData; // used to determine if the state should record its own data or leave it to other functions

    // Helper functions
    void determineAccelerationMagnitude();
    void determineStage();
    bool applySensorType(int i, int sensorNum); // assigns values to the sensor variables.
    void setCsvString(char *str, const char *start, int startSize, bool header);
    bool sensorOK(const Sensor *sensor);

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
    double headingAngle;          // in degrees

    char launchTimeOfDay[9];

    // Kalman Filter settings
    bool useKF;
    void initKF();
    akf::KFState *kfilter;
    // time pos x y z vel x y z acc x y z
    double *predictions;
    // gps x y z barometer z
    double *measurements;
    // imu x y z
    double *inputs;


    //Kalman Filter settings
    bool useKF;
    LinearKalmanFilter *kfilter;
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
    Finally, make sure too update the NUM_MAX_SENSORS, SENSOR_ORDER, and SENSOR_NAMES arrays to reflect the new sensor.

    It's that easy!
    Don't forget to call addSensor() with your new sensor.

    Use sensorOK() to check if a sensor is enabled before using it.

*/