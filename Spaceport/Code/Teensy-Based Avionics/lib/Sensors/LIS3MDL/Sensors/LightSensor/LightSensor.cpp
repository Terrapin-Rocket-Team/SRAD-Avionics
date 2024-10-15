#include "LightSensor.h"

namespace mmfs
{

    const double LightSensor::getLux() const
    {
        return lux;
    }

    const char *LightSensor::getCsvHeader() const
    {
        return "L-Light (lux),";
    }

    const char *LightSensor::getDataString() const
    {
        sprintf(data, "%.2f,", lux); // trailing comma
        return data;
    }

    const char *LightSensor::getStaticDataString() const
    {
        sprintf(staticData, "Initial Light (lux): %.2f\n", initialLux);
        return staticData;
    }
    void LightSensor::update()
    {
        read();
    }
}