#ifndef IMU_H
#define IMU_H

#include "../Sensor.h"
#include "../../Math/Vector.h"
#include "../../Math/Quaternion.h"
#include "../../Constants.h"

namespace mmfs
{

    // TODO:
    // - Implement Global and Local orientations
    // - Implement Global and Local accelerations
    // - Implement Magnetometer reading fusion
    // - Revisit naming conventions
    // - Revisit necessary functions/variables

    class IMU : public Sensor
    {
    public:
        virtual ~IMU(){};

        //local represents the orientation off of the intial angle. Global represents the orientation with repesct to gravity
        virtual Quaternion getOrientationLocal();
        virtual Quaternion getOrientationGlobal();
        virtual Vector<3> getAccelerationLocal();
        virtual Vector<3> getAccelerationGlobal();
        virtual Vector<3> getMagnetometerReading();
        virtual Vector<3> getGyroReading();
        virtual Vector<3> getAccReading();
        virtual SensorType getType() const override { return IMU_; }
        virtual const char *getTypeString() const override { return "IMU"; }
        virtual const char *getCsvHeader() const override;
        virtual const char *getDataString() const override;
        virtual const char *getStaticDataString() const override;
        virtual void update() override;
        virtual bool begin(bool useBiasCorrection = true) override;

    protected:
        IMU()
        {                                       // Protected constructor to prevent instantiation
            staticData = new char[30 + MAX_DIGITS_FLOAT * 3]; // 30 chars for the string, 12 chars for the 3 floats
            data = new char[MAX_DIGITS_FLOAT * 10 + 10];      // 10x floats + buffer space
        };
        // Hardware data
        Vector<3> measuredAcc = Vector<3>(0, 0, 0);
        Vector<3> measuredMag = Vector<3>(0, 0, 0);
        Vector<3> measuredGyro = Vector<3>(0, 0, 0);

        // IMU data
        Vector<3> initialMagField = Vector<3>(0, 0, 0);
        Vector<3> initialAcc = Vector<3>(0, 0, 0);
        Vector<3> GyroOffset = Vector<3>(0, 0, 0);


        // IMU data (calculated)
        Vector<3> accelerationGlobal = Vector<3>(0, 0, 0);
        Vector<3> accelerationLocal = Vector<3>(0, 0, 0);
        Quaternion orientationGlobal = Quaternion(1, 0, 0, 0);
        Quaternion orientationLocal = Quaternion(1, 0, 0, 0);
    };
  
}
#endif