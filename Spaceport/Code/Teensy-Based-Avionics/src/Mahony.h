#ifndef MAHONY_H
#define MAHONY_H

#include <Arduino.h>
#include "Math/Quaternion.h"
#include "Math/Vector.h"
#include "Eigen/Dense"
#include <vector>

// need to add magnetomater to Mahony filter to correct accumulated gyrscope drift. Magnetometer spits 
// out strenght of Earth magnetic field in x, y, z components. Needs its own calibration process with 
// soft and hard iron dependencies. Calibration: MOtioncal (program from pjrc)


using namespace mmfs;

class MahonyAHRS
{
public:


    MahonyAHRS(double Kp = 0.1, double Ki = 0.0005)
        : _Kp(Kp), _Ki(Ki), _biasX(0.0), _biasY(0.0), _biasZ(0.0),
          _sumAccel(), _sumGyro(), _sumMag(), _calibSamples(0), _q(1.0, 0, 0, 0) /*0 rotation quaternion*/, 
          _q0(1.0, 0.0, 0.0, 0.0)/*0 rotation quaternion, but with floats*/, _initialized(false)
    {
    }
    //mag needs its own calibration because it needs dynamic, 3D rotation; static samples aren't enough.
    //using eigen matrices to perform c++ calculation of calibrations, cannot seem to find a way to offload the calibration math.
    //found algorithm to calculate the matrices at: https://github.com/TonyPhh/magnetometer-calibration/blob/master/magnetometer_calibrate.cpp
    //Note: I have downloaded a large repo (Eigen) in order to perform the array/matrices/etc. calculations. It is heavy for 
    //teensy, but it only needs to call it once. If there is another way, please let me know. 
  void calibratedMag( Vector<Vector<3>> &raw) {
    //have not finished implementing the algorithm that spits out the calibration matrices
    int num = raw.size();
    if(num == 0){
        return Vector<3> {0, 0, 0};
    }
    Vector<3> temp;

    // Apply hard-iron offset
    temp.x = raw.x - hard_iron[0];
    temp.y = raw.y - hard_iron[1];
    temp.z = raw.z - hard_iron[2];

    // Apply soft-iron correction
    Vector<3> corrected;
    corrected.x = soft_iron[0][0]*temp.x + soft_iron[0][1]*temp.y + soft_iron[0][2]*temp.z;
    corrected.y = soft_iron[1][0]*temp.x + soft_iron[1][1]*temp.y + soft_iron[1][2]*temp.z;
    corrected.z = soft_iron[2][0]*temp.x + soft_iron[2][1]*temp.y + soft_iron[2][2]*temp.z;

    return corrected;
}


    // Collect static samples before launch
    void calibrate(const Vector<3> &accel, const Vector<3> &gyro)
    {
        _sumAccel += accel;
        _sumGyro += gyro;
        _calibSamples++;
    }

    // Finalize calibration and align "down"
    void initialize()
    {
        if (_calibSamples == 0)
            return;
        Vector<3> avgA = _sumAccel * (1.0 / _calibSamples);
        Vector<3> avgG = _sumGyro * (1.0 / _calibSamples);

        // Set gyro bias
        _biasX = avgG.x();
        _biasY = avgG.y();
        _biasZ = avgG.z();

        // Align initial orientation so body Z matches gravity
        Vector<3> bodyDown = avgA;
        bodyDown.normalize();
        Vector<3> earthDown(0.0, 0.0, 1.0);
        Vector<3> axis = earthDown.cross(bodyDown);
        if (axis.magnitude() < 1e-6)
        {
            axis = Vector<3>(1.0, 0.0, 0.0);
        }
        else
        {
            axis.normalize();
        }
        double dot = bodyDown.dot(earthDown);
        dot = constrain(dot, -1.0, 1.0);
        double angle = acos(dot);
        _q = Quaternion();
        _q.fromAxisAngle(axis, angle);
        _q.normalize();

        // Store initial orientation for relative output
        _q0 = _q;
        _initialized = true;
    }

    /**
     * Update orientation using only accelerometer (for tilt) and gyro.
     * @param accel : Vector<3> accelerometer readings (m/s^2)
     * @param gyro  : Vector<3> gyroscope readings (rad/s)
     * @param dt    : time step (s)
     */
    void update(const Vector<3> &accel,
                const Vector<3> &gyro, 
                const Vector<3> &mag,
                double dt)
    {
        if (!_initialized)
            return;

        // 1. Normalize accelerometer measurement
        Vector<3> a = accel * -1.0; // invert so gravity is positive Z
        a.normalize();

        // 2. Estimated gravity direction in body frame
        Vector<3> vAcc = _q.rotateVector(Vector<3>(0.0, 0.0, 1.0)); // Earth frame gravity vector [0,0,1]

        // 3. Error between measured and estimated gravity
        Vector<3> e = a.cross(vAcc);

        // 4. Update gyroscope bias (integral action)
        _biasX += _Ki * e.x() * dt;
        _biasY += _Ki * e.y() * dt;
        _biasZ += _Ki * e.z() * dt;

        // 5. Corrected angular velocity (gyroscope measurements compensated for bias and error)
        Vector<3> gCorr(
            -gyro.x() - _biasX + _Kp * e.x(),
            -gyro.y() - _biasY + _Kp * e.y(),
            -gyro.z() - _biasZ + _Kp * e.z());

        Quaternion qDot = Quaternion(0.0, gCorr.x(), gCorr.y(), gCorr.z()) * _q * 0.5;

        // 7. Integrate and normalize
        _q = _q + (qDot * dt);
        _q.normalize();
    }

    // Returns the orientation relative to initial alignment
    Quaternion getQuaternion() const
    {
        // compute q_rel = q0*^-1 * q
        Quaternion qInv = _q0.conjugate();
        return qInv * _q;
    }

    Quaternion getAbsoluteQuaternion() const
    {
        return _q;
    }

    Vector<3> toEarthFrame(const Vector<3> &v) const
    {
        Vector<3> ret = _q.conjugate().rotateVector(v) * -1.0;
        ret.z() *= -1.0;
        return ret;
    }

    bool isInitialized() const { return _initialized; }

private:
    double _Kp, _Ki; //tuning parameters for mahony filter
    double _biasX, _biasY, _biasZ; //biases?
    Quaternion _q, _q0; //rotation quaternion 
    Vector<3> _sumAccel, _sumGyro, _sumMag; //vectors 
    int _calibSamples; //number of samples?
    bool _initialized;
};

#endif // MAHONY_H