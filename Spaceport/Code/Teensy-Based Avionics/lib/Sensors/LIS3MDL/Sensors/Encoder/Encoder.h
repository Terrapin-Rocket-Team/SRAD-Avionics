#ifndef ENCODER_H
#define ENCODER_H

#include "../Sensor.h"
#include "../../Constants.h"

namespace mmfs
{
    class Encoder : Sensor
    {

    public:
        virtual ~Encoder() {}
        virtual const char *getCsvHeader() const override;
        virtual const char *getDataString() const override;
        virtual const char *getStaticDataString() const override;
        virtual const char *getTypeString() const override { return "Encoder"; }
        virtual SensorType getType() const override { return ENCODER_; }
        virtual void update() override;
        virtual bool begin(bool useBiasCorrection = true) override;

    protected:
        Encoder()
        {                                       // Protected constructor to prevent instantiation
            staticData = new char[25 + MAX_DIGITS_FLOAT * 1]; // 25 chars for the string, 12 chars for the float
            data = new char[MAX_DIGITS_FLOAT * 1 + 3];        // 12 chars for the float, 3 for the comma
        }
        int currentRelativeSteps;
        int initialSteps;
    };
}
#endif // ENCODER_H