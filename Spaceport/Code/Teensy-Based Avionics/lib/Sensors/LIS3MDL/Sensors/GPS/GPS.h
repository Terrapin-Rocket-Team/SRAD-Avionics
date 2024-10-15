#ifndef GPS_H
#define GPS_H

#include "../Sensor.h"
#include "../../Constants.h"
#include "../../Math/Vector.h"
#include "../../RecordData/Logger.h"

namespace mmfs
{
    class GPS : public Sensor
    {
    public:
        virtual ~GPS(){};
        virtual Vector<3> getPos() const;
        virtual Vector<3> getOrigin() const;
        virtual Vector<3> getDisplacement() const;
        virtual int getFixQual() const;
        virtual double getHeading() const;
        virtual bool getHasFirstFix() const;
        virtual const char *getCsvHeader() const override;
        virtual const char *getDataString() const override;
        virtual const char *getStaticDataString() const override;
        virtual void update() override;
        virtual bool begin(bool useBiasCorrection = true) override;

        virtual const char *getTypeString() const override { return "GPS"; }
        virtual SensorType getType() const override { return GPS_; }

    protected:
        GPS();
        Vector<3> position;          // latitude and longitude, alt
        Vector<3> displacement; // m from starting location
        Vector<3> origin;       // lat(deg), long(deg), alti(m) of the original location
        int fixQual;                 // num of connections to satellites
        bool hasFirstFix;            // whether or not gps has reached at least 3 satellites since restart
        double heading;

        // distance finding
        void calcInitialValuesForDistance();
        double kx, ky;
        void calcDistance();
        double wrapLongitude(double val);

        CircBuffer<Vector<3>> originBuffer = CircBuffer<Vector<3>>(CIRC_BUFFER_LENGTH);
    };
}

#endif