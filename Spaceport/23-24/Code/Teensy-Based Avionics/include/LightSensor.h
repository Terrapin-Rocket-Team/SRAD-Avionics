// Placeholder file for the LightSensor class

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H


class LightSensor : public Sensor {
public:
    virtual ~LightSensor() {}; //Virtual descructor. Very important
    virtual bool calibrate() = 0; //Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual double get_pressure() = 0;
    virtual double get_temp() = 0;
    virtual double get_temp_f() = 0;
    virtual double get_pressure_atm() = 0;
    virtual double get_rel_alt_ft() = 0;
};


#endif 