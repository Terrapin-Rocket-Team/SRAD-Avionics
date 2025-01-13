#include "AvionicsState.h"
#include "IMU.h"
#include "Barometer.h"
#include "GPS.h"
#include "Logger.h"
#include "BlackBox.h"

extern Logger logger;
extern BlackBox bb;

AvionicsState::AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter) : State(sensors, numSensors, kfilter)
{
    stage = 0;
    timeOfLaunch = 0;
    timeOfLastStage = 0;
    timeOfDay = 0;
}

void AvionicsState::updateState(double newTime)
{
    currentTime = (newTime < 0) ? millis() / 1000.0 : newTime;

    for (int i = 0; i < maxNumSensors; i++)
    {
        if (sensors[i] != nullptr)
        {
            sensors[i]->update(currentTime);
        }
    }

    determineStage();
    determineIfBallistic();  // Add this call to check for ballistic descent
    packData();
}

void AvionicsState::determineStage()
{
    IMU *imu = reinterpret_cast<IMU *>(getSensor(IMU_));
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));
    GPS *gps = reinterpret_cast<GPS *>(getSensor(GPS_));

    if (stage == 0 &&
        (sensorOK(imu) || sensorOK(baro)) &&
        (sensorOK(baro) ? baro->getAGLAltFt() > 30 : true))
    {
        // if we are in preflight AND
        // we have either the IMU OR the barometer AND
        // imu is ok AND the z acceleration is greater than 29 ft/s^2 OR imu is not ok AND
        // barometer is ok AND the relative altitude is greater than 30 ft OR baro is not ok

        // essentially, if we have either sensor and they meet launch threshold, launch. 
        // Otherwise, it will never detect a launch.

        logger.setRecordMode(FLIGHT);
        bb.aonoff(33, 200);
        stage = 1;
        timeOfLaunch = currentTime;
        timeOfLastStage = currentTime;
        logger.recordLogData(INFO_, "Launch detected.");
        logger.recordLogData(INFO_, "Printing static data.");
        
        for (int i = 0; i < maxNumSensors; i++)
        {
            if (sensorOK(sensors[i]))
            {
                // char logData[200];
                // snprintf(logData, 200, "%s: %s", sensors[i]->getName(), sensors[i]->getStaticDataString());
                // logger.recordLogData(INFO_, logData);
                sensors[i]->setBiasCorrectionMode(false);
            }
        }
    } // TODO: Add checks for each sensor being ok and decide what to do if they aren't.

    if (stage == 1 && currentTime - timeOfLastStage > 3.0)
    {
        stage = 2;
        timeOfLastStage = currentTime;
        logger.recordLogData(INFO_, "Motor burnout detected.");
    }

    if (stage == 2 && baro != nullptr && sensorOK(baro))
    {
        static float maxAlt = 0;
        static float prevAlt = baro->getAGLAltFt();
        float currAlt = baro->getAGLAltFt();

        if (currAlt > maxAlt)
        {
            maxAlt = currAlt;
        }

        if (maxAlt > 100 && currAlt < prevAlt - 20)
        {
            stage = 3;
            timeOfLastStage = currentTime;
            logger.recordLogData(INFO_, "Apogee detected.");
        }

        prevAlt = currAlt;
    }

    if (stage == 3 && baro != nullptr && sensorOK(baro))
    {
        float alt = baro->getAGLAltFt();
        if (alt < 1500 && alt > 0)
        {
            stage = 4;
            timeOfLastStage = currentTime;
            logger.recordLogData(INFO_, "Main deployment altitude reached.");
        }
    }

    if (stage == 4 && baro != nullptr && sensorOK(baro))
    {
        float alt = baro->getAGLAltFt();
        if (alt < 10)
        {
            stage = 5;
            timeOfLastStage = currentTime;
            logger.recordLogData(INFO_, "Landing detected.");
            logger.setRecordMode(GROUND);
        }
    }
}

void AvionicsState::determineIfBallistic()
{
    IMU *imu = reinterpret_cast<IMU *>(getSensor(IMU_));
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));

    if (!imu || !baro || !sensorOK(imu) || !sensorOK(baro))
    {
        logger.recordLogData(WARNING_, "One or more sensors unavailable. Ballistic check skipped.");
        return;
    }

    // Only check for ballistic descent after drogue deployment stage
    if (stage >= 3)
    {
        float accelerationZ = imu->getAccelerationGlobal().z();
        float baroAlt = baro->getAGLAltFt();
        
        // Calculate vertical velocity using barometer
        static float prevBaroAlt = baroAlt;
        static unsigned long prevTime = currentTime;
        float timeDelta = (currentTime - prevTime);
        float baroVel = (baroAlt - prevBaroAlt) / timeDelta;
        
        prevBaroAlt = baroAlt;
        prevTime = currentTime;

        // Check for ballistic conditions:
        // 1. Low vertical acceleration (near free-fall)
        // 2. High downward velocity
        if (abs(accelerationZ) < 1.0 && baroVel < -50)
        {
            logger.recordLogData(WARNING_, "Ballistic descent detected - saving data");
            logger.setRecordMode(GROUND);
            saveDataToSDCard();
        }
    }
}

void AvionicsState::saveDataToSDCard()
{
    // Implementation depends on your SD card setup and data format
    // Here's a basic implementation:
    logger.recordLogData(WARNING_, "Emergency data save initiated");
    
    // Save current state data
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Stage: %s, Time since launch: %.2f", 
             stages[stage], currentTime - timeOfLaunch);
    logger.recordLogData(INFO_, buffer);

    // Force flush of any buffered data
    logger.flush();
}

void AvionicsState::packData()
{
    IMU *imu = reinterpret_cast<IMU *>(getSensor(IMU_));
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));
    GPS *gps = reinterpret_cast<GPS *>(getSensor(GPS_));

    int i = 0;

    packedData[i++] = currentTime;
    packedData[i++] = stage;
    packedData[i++] = timeOfLaunch;

    if (imu != nullptr && sensorOK(imu))
    {
        Vector3D acc = imu->getAccelerationGlobal();
        Vector3D gyro = imu->getAngularVelocity();
        Vector3D mag = imu->getMagneticField();
        Quaternion quat = imu->getQuaternion();

        packedData[i++] = acc.x();
        packedData[i++] = acc.y();
        packedData[i++] = acc.z();
        packedData[i++] = gyro.x();
        packedData[i++] = gyro.y();
        packedData[i++] = gyro.z();
        packedData[i++] = mag.x();
        packedData[i++] = mag.y();
        packedData[i++] = mag.z();
        packedData[i++] = quat.w();
        packedData[i++] = quat.x();
        packedData[i++] = quat.y();
        packedData[i++] = quat.z();
    }
    else
    {
        for (int j = 0; j < 13; j++)
        {
            packedData[i++] = 0;
        }
    }

    if (baro != nullptr && sensorOK(baro))
    {
        packedData[i++] = baro->getPressure();
        packedData[i++] = baro->getTemperature();
        packedData[i++] = baro->getAGLAltFt();
    }
    else
    {
        for (int j = 0; j < 3; j++)
        {
            packedData[i++] = 0;
        }
    }

    if (gps != nullptr && sensorOK(gps))
    {
        packedData[i++] = gps->getLatitude();
        packedData[i++] = gps->getLongitude();
        packedData[i++] = gps->getAltitude();
        packedData[i++] = gps->getSpeed();
        packedData[i++] = gps->getCourse();
        packedData[i++] = gps->getHDOP();
        packedData[i++] = gps->getNumSatellites();
    }
    else
    {
        for (int j = 0; j < 7; j++)
        {
            packedData[i++] = 0;
        }
    }
}

const PackedType *AvionicsState::getPackedOrder() const
{
    static const PackedType order[] = {
        DOUBLE,  // currentTime
        UINT32,  // stage
        DOUBLE,  // timeOfLaunch
        FLOAT,   // acc.x
        FLOAT,   // acc.y
        FLOAT,   // acc.z
        FLOAT,   // gyro.x
        FLOAT,   // gyro.y
        FLOAT,   // gyro.z
        FLOAT,   // mag.x
        FLOAT,   // mag.y
        FLOAT,   // mag.z
        FLOAT,   // quat.w
        FLOAT,   // quat.x
        FLOAT,   // quat.y
        FLOAT,   // quat.z
        FLOAT,   // pressure
        FLOAT,   // temperature
        FLOAT,   // altitude
        DOUBLE,  // latitude
        DOUBLE,  // longitude
        FLOAT,   // gps altitude
        FLOAT,   // speed
        FLOAT,   // course
        FLOAT,   // hdop
        UINT8    // numSatellites
    };
    return order;
}

const int AvionicsState::getNumPackedDataPoints() const
{
    return 26;
}

const char **AvionicsState::getPackedDataLabels() const
{
    static const char *labels[] = {
        "Time (s)",
        "Stage",
        "Launch Time (s)",
        "Acceleration X (m/s^2)",
        "Acceleration Y (m/s^2)",
        "Acceleration Z (m/s^2)",
        "Angular Velocity X (rad/s)",
        "Angular Velocity Y (rad/s)",
        "Angular Velocity Z (rad/s)",
        "Magnetic Field X (uT)",
        "Magnetic Field Y (uT)",
        "Magnetic Field Z (uT)",
        "Quaternion W",
        "Quaternion X",
        "Quaternion Y",
        "Quaternion Z",
        "Pressure (Pa)",
        "Temperature (C)",
        "Altitude AGL (ft)",
        "Latitude (deg)",
        "Longitude (deg)",
        "GPS Altitude (m)",
        "GPS Speed (m/s)",
        "GPS Course (deg)",
        "GPS HDOP",
        "GPS Satellites"
    };
    return labels;
}
