#include <Arduino.h>
#include "Math/Quaternion.h"
#include "Math/Vector.h"

using namespace mmfs;

class MahonyAHRS
{
public:
    MahonyAHRS(double Kp = 0.1, double Ki = 0.0005)
        : _Kp(Kp), _Ki(Ki), _biasX(0.0), _biasY(0.0), _biasZ(0.0),
          _sumAccel(), _sumGyro(), _calibSamples(0), _q(1.0, 0, 0, 0), _q0(1.0, 0.0, 0.0, 0.0), _initialized(false)
    {
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
    double _Kp, _Ki;
    double _biasX, _biasY, _biasZ;
    Quaternion _q, _q0;
    Vector<3> _sumAccel, _sumGyro;
    int _calibSamples;
    bool _initialized;
};
