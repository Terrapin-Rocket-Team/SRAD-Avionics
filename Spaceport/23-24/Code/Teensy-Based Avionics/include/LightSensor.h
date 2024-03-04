// Placeholder file for the LightSensor class

#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H


class LightSensor : public Sensor {
public:
    virtual ~LightSensor() {}; //Virtual descructor. Very important
    virtual bool calibrate() = 0; //Virtual functions set equal to zero are "pure virtual functions". (like abstract functions in Java)
    virtual double getPressure() = 0;
    virtual double getTemp() = 0;
    virtual double getTempF() = 0;
    virtual double getPressureAtm() = 0;
    virtual double getRelAltFt() = 0;
    virtual SensorType getType() override { return LIGHT_SENSOR_; }
    virtual const char *getTypeString() override { return "Light Sensor"; }
};


#endif 