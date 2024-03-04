#ifndef SENSOR_H
#define SENSOR_H

#include "RecordData.h"


enum SensorType {
    BAROMETER_,
    GPS_,
    IMU_,
    LIGHT_SENSOR_,
    RADIO_,
    RTC_,
    UNKNOWN_
};
class Sensor {
public:
    virtual ~Sensor() {}; //Virtual descructor. Very important
    // Sets up the sensor and stores any critical parameters
    virtual bool initialize() = 0; 
    // a generic function that returns a pointer to the sensor's most import data value
    //deprecate this function?
    virtual void * getData() = 0;
    // gives the names of the columns which transient data will be stored under, in a comma separated string
    virtual const char* getCsvHeader() = 0;
    // gives the data values of the variables said to be stored by the header, in the same order, in a comma separated string
    virtual char* getDataString() = 0;//HEAP ALLOCATED
    // gives a string which includes all the sensor's static/initialization data. This will be in the format of dataName:dataValue, with each pair separated by a newline (\n)
    virtual char* getStaticDataString() = 0;//HEAP ALLOCATED

    virtual SensorType getType() = 0;//Returns the type of the sensor
    virtual const char* getTypeString() = 0;//Returns the type of the sensor as a string
    virtual const char* getName() = 0;//Returns the name of the sensor

    virtual void update() = 0;//Updates the sensor's fields by querying the sensor for new data
};


#endif //SENSOR_H