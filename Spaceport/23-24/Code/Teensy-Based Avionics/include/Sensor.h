#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>


class Sensor {
public:
    virtual ~Sensor() {}; //Virtual descructor. Very important
    // Sets up the sensor and stores any critical parameters
    virtual bool initialize() = 0; 
    // a generic function that returns a pointer to the sensor's most import data value
    virtual void * get_data() = 0;
    // gives the names of the columns which transient data will be stored under, in a comma separated string
    virtual char* getcsvHeader() = 0;
    // gives the data values of the variables said to be stored by the header, in the same order, in a comma separated string
    virtual char* getdataString() = 0;
    // gives a string which includes all the sensor's static/initialization data. This will be in the format of dataName:dataValue, with each pair separated by a newline (\n)
    virtual char* getStaticDataString() = 0;
};


#endif //SENSOR_H