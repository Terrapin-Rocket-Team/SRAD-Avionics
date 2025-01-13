#include "AvionicsState.h"
#include "MMFS.h"  // Include for MMFS and the mmfs namespace
#include "SD.h"    // Include for SD card functionality (if you're using it for saving data)

using namespace mmfs;  // Using the mmfs namespace

AvionicsState::AvionicsState(Sensor **sensors, int numSensors, LinearKalmanFilter *kfilter) : State(sensors, numSensors, kfilter)
{
    stage = 0;
    timeOfLaunch = 0;
    timeOfLastStage = 0;
    timeOfDay = 0;
}

void AvionicsState::updateState(double newTime)
{
    State::updateState(newTime); // Call base version for sensor updates
    determineStage();            // Determine the stage of the flight
    determineIfBallistic();      // Check for ballistic descent
}

void AvionicsState::determineStage()
{
    int timeSinceLaunch = currentTime - timeOfLaunch;
    IMU *imu = reinterpret_cast<IMU *>(getSensor(IMU_));
    Barometer *baro = reinterpret_cast<Barometer *>(getSensor(BAROMETER_));

    if (stage == 0 &&
        (sensorOK(imu) || sensorOK(baro)) &&
        (sensorOK(baro) ? baro->getAGLAltFt() > 30 : true))
    {

// if we are in preflight AND
    // we have either the IMU OR the barometer AND
    // imu is ok AND the z acceleration is greater than 29 ft/s^2 OR imu is not ok AND
    // barometer is ok AND the relative altitude is greater than 30 ft OR baro is not ok

    // essentially, if we have either sensor and they meet launch threshold, launch. Otherwise, it will never detect a launch.

        
        logger.setRecordMode(FLIGHT);
        bb.aonoff(33, 200);
        // logger.setRecordMode(FLIGHT);
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
    }    // TODO: Add checks for each sensor being ok and decide what to do if they aren't.


    else if (stage == 1 && abs(acceleration.z()) < 10)
    {
        bb.aonoff(33, 200, 2);
        timeOfLastStage = currentTime;
        stage = 2;
        logger.recordLogData(INFO_, "Coasting detected.");
    }
    else if (stage == 2 && baroVelocity <= 0 && timeSinceLaunch > 5)
    {
        bb.aonoff(33, 200, 3);
        char logData[100];
        snprintf(logData, 100, "Apogee detected at %.2f m.", position.z());
        logger.recordLogData(INFO_, logData);
        timeOfLastStage = currentTime;
        stage = 3;
        logger.recordLogData(INFO_, "Drogue conditions detected.");
    }
    else if (stage == 3 && baro->getAGLAltFt() < 1000 && timeSinceLaunch > 10)
    {
        bb.aonoff(33, 200, 4);
        stage = 4;
        timeOfLastStage = currentTime;
        logger.recordLogData(INFO_, "Main parachute conditions detected.");
    }
    else if (stage == 4 && baroVelocity > -1 && baro->getAGLAltFt() < 66 && timeSinceLaunch > 15)
    {
        bb.aonoff(33, 200, 5);
        timeOfLastStage = currentTime;
        stage = 5;
        logger.recordLogData(INFO_, "Landing detected. Waiting for 5 seconds to dump data.");
    }
    else if ((stage == 5 && currentTime - timeOfLastStage > 5) || (stage >= 1 && timeSinceLaunch > 5 * 60))
    {
        stage = 6;
        logger.setRecordMode(GROUND);
        logger.recordLogData(INFO_, "Dumped data after landing.");
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

    float accelerationZ = imu->getAccelerationGlobal().z();
    
    // Attempt to estimate velocity using barometer pressure change
    float baroAlt = baro->getAGLAltFt();  // Altitude in feet
    static float prevBaroAlt = 0;
    static unsigned long prevTime = currentTime;

    float baroVel = (baroAlt - prevBaroAlt) / (currentTime - prevTime);  // Approximate velocity

    prevBaroAlt = baroAlt;
    prevTime = currentTime;

    if (stage >= 3 && abs(accelerationZ) < 1.0 && baroVel < -50) // Ballistic check: low acceleration and downward velocity
    {
        logger.recordLogData(WARNING_, "Ballistic descent detected. Saving data and switching to data dump mode.");
        
        // Save current data to PSRAM (simulate saving it)
        psram->setMode(GROUND);  // Use -> to access the member function of psram (pointer)

        // Save data to SD card (dump current flight data)
        saveDataToSDCard();  // Implement this function for saving data
    }
}

void AvionicsState::saveDataToSDCard()
{
    // Function to save the packed data (currently in PSRAM) to the SD card
    char fileName[] = "/rocket_data.txt"; // Define the file name

    // Open the file for writing
    SD.begin(10);  // Assuming 10 is the CS pin for SD card
    File dataFile = SD.open(fileName, FILE_WRITE);

    if (dataFile)
    {
        // Write each data point to the SD card (simulating packing of data)
        for (int i = 0; i < getNumPackedDataPoints(); i++)
        {
            dataFile.print(getPackedDataLabels()[i]);
            dataFile.print(": ");
            dataFile.println(getPackedData()[i]);  // Assuming getPackedData() returns the packed data
        }

        dataFile.close();  // Close the file after writing
        logger.recordLogData(INFO_, "Data saved to SD card.");
    }
    else
    {
        logger.recordLogData(ERROR_, "Failed to open SD card for writing.");
    }
}

const int AvionicsState::getNumPackedDataPoints() const { return 11; }

const PackedType *AvionicsState::getPackedOrder() const
{
    static const PackedType order[11] = {
        FLOAT, INT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT};
    return order;
}

const char **AvionicsState::getPackedDataLabels() const
{
    static const char *labels[] = {
        "Time (s)",
        "Stage",
        "PX (m)",
        "PY (m)",
        "PZ (m)",
        "VX (m/s)",
        "VY (m/s)",
        "VZ (m/s)",
        "AX (m/s/s)",
        "AY (m/s/s)",
        "AZ (m/s/s)"};
    return labels;
}

void AvionicsState::packData()
{
    float t = currentTime;
    float px = position.x();
    float py = position.y();
    float pz = position.z();
    float vx = velocity.x();
    float vy = velocity.y();
    float vz = velocity.z();
    float ax = acceleration.x();
    float ay = acceleration.y();
    float az = acceleration.z();

    int cursor = 0;
    memcpy(packedData + cursor, &t, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &stage, sizeof(int));
    cursor += sizeof(int);
    memcpy(packedData + cursor, &px, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &py, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &pz, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &vx, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &vy, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &vz, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &ax, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &ay, sizeof(float));
    cursor += sizeof(float);
    memcpy(packedData + cursor, &az, sizeof(float));
    cursor += sizeof(float);
}
