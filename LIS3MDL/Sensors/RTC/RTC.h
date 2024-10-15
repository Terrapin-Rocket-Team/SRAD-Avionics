#ifndef RTC_H
#define RTC_H

#include <RTClib.h>
#include "../Sensor.h"
#include "../../Math/Vector.h"

// NOT READY FOR IMPLEMENTATION. NNEDS TO BE REWORKED TO MATCH OTHER SENSORS LIKE BARO, IMU, OR GPS

namespace mmfs
{
    class RTC : public Sensor
    {
    public:
        virtual ~RTC(){};                                 // virtual destructor
        virtual Vector<2> getTimeOn() = 0;          // ms
        virtual Vector<2> getTimeSinceLaunch() = 0; // ms
        virtual DateTime getLaunchTime() = 0;
        virtual DateTime setLaunchTime() = 0;
        virtual DateTime getPowerOnTime() = 0;
        virtual DateTime getCurrentTime() = 0;
        virtual SensorType getType() const override { return RTC_; }
        virtual const char *getTypeString() const override { return "RTC"; }
    };
} // namespace mmfs
#endif
