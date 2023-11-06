#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>


class Sensor {
public:
    virtual ~Sensor() {}; //Virtual descructor. Very important
    virtual void initialize() = 0; 
    virtual void * get_data() = 0;
    virtual String getcsvHeader() = 0;
    virtual String getdataString() = 0;
};


#endif //SENSOR_H