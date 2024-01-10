#include "BNO055.h"


BNO055::BNO055(uint8_t SCK, uint8_t SDA) {
    SCK_pin = SCK;
    SDA_pin = SDA;
}

void BNO055::initialize()
{
    if (!bno.begin())
    {
        while (1);
    }
    bno.setExtCrystalUse(true);

    initial_mag_field = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
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


imu::Quaternion BNO055::get_orientation() {
    return bno.getQuat();
}

imu::Vector<3> BNO055::get_acceleration() {
    acceleration_vec = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    return acceleration_vec;
}

imu::Vector<3> BNO055::get_orientation_euler() {
    orientation_euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    return orientation_euler;
}

imu::Vector<3> BNO055::get_magnetometer() {
    magnetometer = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
    return magnetometer;
}

imu::Vector<3> convert_to_euler(imu::Quaternion orientation) {
    imu::Vector<3> euler = orientation.toEuler();
    // reverse the vector, since it returns in z, y, x
    euler = imu::Vector<3>(euler.x(), euler.y(), euler.z());
    return euler;
}

// will give back a pointer to a linear acceleration vector
void * BNO055::get_data() {
    return (void *) &acceleration_vec;
}

String BNO055::getcsvHeader() {
    return "acceleration_X (m/s/s), acceleration_Y (m/s/s), acceleration_Z (m/s/s), euler X, euler Y, euler Z, Quaternion X, Quaternion Y, Quaternion Z, Quaternion W";
}

String BNO055::getdataString() {
    return String(acceleration_vec.x()) + "," + String(acceleration_vec.y()) + "," + String(acceleration_vec.z()) + "," + String(orientation_euler.x()) + "," + String(orientation_euler.y()) + "," + String(orientation_euler.z()) + "," + String(orientation.x()) + "," + String(orientation.y()) + "," + String(orientation.z()) + "," + String(orientation.w());
}

String BNO055::getStaticDataString() {
    return "Initial Magnetic Field (uT):" + String(initial_mag_field.x()) + "," + String(initial_mag_field.y()) + "," + String(initial_mag_field.z()) + "\n";
}



