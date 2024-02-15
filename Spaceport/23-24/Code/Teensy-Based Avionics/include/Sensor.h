#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h> //only still needed for getStaticDataString. Will remove when that has been rewritten.
#include "Utils.h"
class Sensor
{
public:
    virtual ~Sensor(){}; // Virtual descructor. Very important
    // Sets up the sensor and stores any critical parameters
    virtual void initialize() = 0;
    // a generic function that returns a pointer to the sensor's most import data value
    virtual void *get_data() = 0;


    //UNSAFE METHODS - DO NOT FORGET TO CLEAR THE MEMORY THEY USE WHEN YOU ARE DONE
    //------------------------
    // gives the names of the columns which transient data will be stored under, as an array of character arrays
    virtual char **getcsvHeader() = 0;
    // gives the data values of the variables said to be stored by the header, in the same order, as an array of character arrays
    virtual char **getdataString() = 0;
    //------------------------


    // gives a string which includes all the sensor's static/initialization data. This will be in the format of dataName:dataValue, with each pair separated by a newline (\n)
    virtual String getStaticDataString() = 0;
    virtual int getNumDatapoints() = 0;
protected:
    int datapoints;//how many data points are returned when you ask for getcsvHeader or getdataString
};

#endif // SENSOR_H