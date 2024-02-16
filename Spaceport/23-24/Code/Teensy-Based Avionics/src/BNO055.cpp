#include "BNO055.h"

BNO055::BNO055(uint8_t SCK, uint8_t SDA)
{
    SCK_pin = SCK;
    SDA_pin = SDA;
}

bool BNO055::initialize()
{
    if (!bno.begin())
    {
        return false;
    }
    bno.setExtCrystalUse(true);

    initial_mag_field = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
    return true;
}

void BNO055::calibrate_bno()
{
    uint8_t system, gyro, accel, mag, i = 0;
    while ((system != 3) || (gyro != 3) || (accel != 3) || (mag != 3))
    {
        bno.getCalibration(&system, &gyro, &accel, &mag);
        i = i + 1;
        if (i == 10)
        {
            i = 0;
        }
        delay(10);
    }
}

imu::Quaternion BNO055::get_orientation()
{
    orientation = bno.getQuat();
    return orientation;
}

imu::Vector<3> BNO055::get_acceleration()
{
    acceleration_vec = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    return acceleration_vec;
}

imu::Vector<3> BNO055::get_orientation_euler()
{
    orientation_euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    return orientation_euler;
}

imu::Vector<3> BNO055::get_magnetometer()
{
    magnetometer = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
    return magnetometer;
}

imu::Vector<3> convert_to_euler(imu::Quaternion orientation)
{
    imu::Vector<3> euler = orientation.toEuler();
    // reverse the vector, since it returns in z, y, x
    euler = imu::Vector<3>(euler.x(), euler.y(), euler.z());
    return euler;
}

// will give back a pointer to a linear acceleration vector
void *BNO055::get_data()
{
    return (void *)&acceleration_vec;
}

char *BNO055::getcsvHeader()
{                                                                                                          // incl I- for IMU
    return "I-AX (m/s/s),I-AY (m/s/s),I-AZ (m/s/s),I-ULRX,I-ULRY,I-ULRZ,I-QUATX,I-QUATY,I-QUATZ,I-QUATW,"; // trailing comma
}

char *BNO055::getdataString()
{
    // See State.cpp::setdataString() for comments on what these numbers mean
    const int size = 12*10 + 10;
    char data[size];
    snprintf(data, size, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", acceleration_vec.x(), acceleration_vec.y(), acceleration_vec.z(), orientation_euler.x(), orientation_euler.y(), orientation_euler.z(), orientation.x(), orientation.y(), orientation.z(), orientation.w()); // trailing comma"
    return data;
}

char *BNO055::getStaticDataString()
{
    // See State.cpp::setdataString() for comments on what these numbers mean
    const int size = 30 + 12 * 3;
    char data[size];
    snprintf(data, size, "Initial Magnetic Field (uT): %.2f,%.2f,%.2f\n", initial_mag_field.x(), initial_mag_field.y(), initial_mag_field.z());
    return data;
}
