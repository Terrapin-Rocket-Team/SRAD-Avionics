#include "BNO055.h"

bool BNO055::initialize()
{
    if (!bno.begin())
    {
        return initialized = false;
    }

    // set to +-16g range
    adafruit_bno055_opmode_t mode = bno.getMode();
    bno.setMode(OPERATION_MODE_CONFIG);
    delay(25);

    Wire.beginTransmission(0x29); // BNO055 address
    Wire.write(0x08);             // ACC_CONFIG register address on BNO055
    Wire.write(0b11);             // set to +-16g range
    Wire.endTransmission(true);   // send stop

    recordLogData(INFO, "BNO055 Calibration Loading");
    recordLogData(INFO, "Pre-Loading Values:");
    adafruit_bno055_offsets_t sensorOffsets;
    
    if(!bno.getSensorOffsets(sensorOffsets))
    {
        recordLogData(INFO, "Failed to get sensor offsets", TO_USB);
    }
    char data[100];

    snprintf(data, 100, "ACC: %hd|%hd|%hd|%hd", sensorOffsets.accel_offset_x, sensorOffsets.accel_offset_y, sensorOffsets.accel_offset_z, sensorOffsets.accel_radius);
    recordLogData(INFO, data);
    snprintf(data, 100, "GYR: %hd|%hd|%hd", sensorOffsets.gyro_offset_x, sensorOffsets.gyro_offset_y, sensorOffsets.gyro_offset_z);
    recordLogData(INFO, data);
    snprintf(data, 100, "MAG: %hd|%hd|%hd|%hd", sensorOffsets.mag_offset_x, sensorOffsets.mag_offset_y, sensorOffsets.mag_offset_z, sensorOffsets.mag_radius);
    recordLogData(INFO, data);

    setCalibrationFromFile();

    recordLogData(INFO, "Post-Loading Values:");
    if (!bno.getSensorOffsets(sensorOffsets))
    {
        recordLogData(INFO, "Failed to get sensor offsets", TO_USB);
    }

    snprintf(data, 100, "ACC: %hd|%hd|%hd|%hd", sensorOffsets.accel_offset_x, sensorOffsets.accel_offset_y, sensorOffsets.accel_offset_z, sensorOffsets.accel_radius);
    recordLogData(INFO, data);
    snprintf(data, 100, "GYR: %hd|%hd|%hd", sensorOffsets.gyro_offset_x, sensorOffsets.gyro_offset_y, sensorOffsets.gyro_offset_z);
    recordLogData(INFO, data);
    snprintf(data, 100, "MAG: %hd|%hd|%hd|%hd", sensorOffsets.mag_offset_x, sensorOffsets.mag_offset_y, sensorOffsets.mag_offset_z, sensorOffsets.mag_radius);
    recordLogData(INFO, data);

    bno.setExtCrystalUse(true);

    bno.setMode(mode);
    delay(25);

    initialMagField = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
    imu::Vector<3> read = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    for (int i = 0; i < 20; i++)
    {
        prevReadings[i] = read;
    }
    return initialized = true;
}

void BNO055::update()
{
    if (biasCorrectionMode)
    {
        imu::Vector<3> read = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
        imu::Vector<3> sum = 0;
        for (int i = 0; i < 19; i++)
        {
            prevReadings[i] = prevReadings[i + 1];
            if (i < 10) // ignore last 2 readings to avoid accidentally including launch readings
                sum = sum + prevReadings[i];
        }
        prevReadings[19] = read;
        accelerationVec = read - (sum / 10.0);

    }
    else
    {
        accelerationVec = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    }
    printf("Acceleration: %.2f, %.2f, %.2f\n", accelerationVec.x(), accelerationVec.y(), accelerationVec.z());
    orientation = bno.getQuat();
    orientationEuler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    magnetometer = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);


}

imu::Quaternion BNO055::getOrientation()
{
    return orientation;
}

imu::Vector<3> BNO055::getAcceleration()
{
    return accelerationVec;
}

imu::Vector<3> BNO055::getOrientationEuler()
{
    return orientationEuler;
}

imu::Vector<3> BNO055::getMagnetometer()
{
    return magnetometer;
}

const char *BNO055::getCsvHeader()
{                                                                                                          // incl I- for IMU
    return "I-AX (m/s/s),I-AY (m/s/s),I-AZ (m/s/s),I-ULRX,I-ULRY,I-ULRZ,I-QUATX,I-QUATY,I-QUATZ,I-QUATW,"; // trailing comma
}

char *BNO055::getDataString()
{
    // See State.cpp::setDataString() for comments on what these numbers mean
    const int size = 12 * 10 + 10;
    char *data = new char[size];
    snprintf(data, size, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", accelerationVec.x(), accelerationVec.y(), accelerationVec.z(), orientationEuler.x(), orientationEuler.y(), orientationEuler.z(), orientation.x(), orientation.y(), orientation.z(), orientation.w()); // trailing comma"
    return data;
}

char *BNO055::getStaticDataString()
{
    char *data = new char[100];
    adafruit_bno055_offsets_t sensorOffsets;
    bno.getSensorOffsets(sensorOffsets);
    snprintf(data, 100, "ACC: %hd|%hd|%hd|%hd\nGYR: %hd|%hd|%hd\nMAG: %hd|%hd|%hd|%hd",
             sensorOffsets.accel_offset_x, sensorOffsets.accel_offset_y, sensorOffsets.accel_offset_z, sensorOffsets.accel_radius,
             sensorOffsets.gyro_offset_x, sensorOffsets.gyro_offset_y, sensorOffsets.gyro_offset_z,
             sensorOffsets.mag_offset_x, sensorOffsets.mag_offset_y, sensorOffsets.mag_offset_z, sensorOffsets.mag_radius);
    return data;
}

const char *BNO055::getName()
{
    return "BNO055";
}

void BNO055::setBiasCorrectionMode(bool mode)
{
    biasCorrectionMode = mode;
}

void BNO055::setCalibrationFromFile()
{
    /*
        File format:

        ACC: X|Y|Z|R
        GYR: X|Y|Z
        MAG: X|Y|Z|R
    */
    char data[100];
    readCalibrationData(data, 100);
    adafruit_bno055_offsets_t sensorOffsets;
    sscanf(data, "ACC: %hd|%hd|%hd|%hd\nGYR: %hd|%hd|%hd\nMAG: %hd|%hd|%hd|%hd",
           &sensorOffsets.accel_offset_x, &sensorOffsets.accel_offset_y, &sensorOffsets.accel_offset_z, &sensorOffsets.accel_radius,
           &sensorOffsets.gyro_offset_x, &sensorOffsets.gyro_offset_y, &sensorOffsets.gyro_offset_z,
           &sensorOffsets.mag_offset_x, &sensorOffsets.mag_offset_y, &sensorOffsets.mag_offset_z, &sensorOffsets.mag_radius);
    printf("ACC: %hd|%hd|%hd|%hd\nGYR: %hd|%hd|%hd\nMAG: %hd|%hd|%hd|%hd\n",
           sensorOffsets.accel_offset_x, sensorOffsets.accel_offset_y, sensorOffsets.accel_offset_z, sensorOffsets.accel_radius,
           sensorOffsets.gyro_offset_x, sensorOffsets.gyro_offset_y, sensorOffsets.gyro_offset_z,
           sensorOffsets.mag_offset_x, sensorOffsets.mag_offset_y, sensorOffsets.mag_offset_z, sensorOffsets.mag_radius);
    bno.setSensorOffsets(sensorOffsets);
}

void BNO055::recordCalibrationValues()
{
    adafruit_bno055_offsets_t sensorOffsets;
    bno.getSensorOffsets(sensorOffsets);
    char data[100];
    snprintf(data, 100, "ACC: %hd|%hd|%hd|%hd\nGYR: %hd|%hd|%hd\nMAG: %hd|%hd|%hd|%hd",
             sensorOffsets.accel_offset_x, sensorOffsets.accel_offset_y, sensorOffsets.accel_offset_z, sensorOffsets.accel_radius,
             sensorOffsets.gyro_offset_x, sensorOffsets.gyro_offset_y, sensorOffsets.gyro_offset_z,
             sensorOffsets.mag_offset_x, sensorOffsets.mag_offset_y, sensorOffsets.mag_offset_z, sensorOffsets.mag_radius);
    writeCalibrationData(data);
}

void BNO055::getSensorOffsets(adafruit_bno055_offsets_t &offsets)
{
    bno.getSensorOffsets(offsets);
}

void BNO055::getCalibrationStatus(uint8_t &system, uint8_t &gyro, uint8_t &accel, uint8_t &mag)
{
    bno.getCalibration(&system, &gyro, &accel, &mag);
}