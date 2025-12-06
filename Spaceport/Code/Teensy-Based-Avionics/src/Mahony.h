#ifndef MAHONY_H
#define MAHONY_H

#include <Arduino.h>
#include "Math/Quaternion.h"
#include "Math/Vector.h"
#include "Eigen/Dense"
#include <vector>

// need to add magnetomater to Mahony filter to correct accumulated gyrscope drift. Magnetometer spits 
// out strenght of Earth magnetic field in x, y, z components. Has its own calibration process with 
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


//used to clear the temporary buffer

    void _bufferClear(){
        if(_magCalibrated){
            _magCalibBuffer.clear();
            _magCalibBuffer.shrink_to_fit();
        }
    }

    
    //mag needs its own calibration because it needs dynamic, 3D rotation; static samples aren't enough.
    //using eigen matrices to perform c++ calculation of calibrations, cannot seem to find a way to offload the calibration math.
    //found algorithm to calculate the matrices at: https://github.com/TonyPhh/magnetometer-calibration/blob/master/magnetometer_calibrate.cpp
    //Note: I have downloaded a large repo (Eigen) in order to perform the array/matrices/etc. calculations. It is heavy for 
    //teensy, but it only needs to call it once. If there is another way, please let me know. 


 // Compute magnetometer calibration matrices from collected samples
// This implements ellipsoid fitting to find hard and soft iron corrections
void computeMagCalibration() {
    int data_nums = _magCalibBuffer.size();
    
    // Need sufficient samples for calibration (at least 9 parameters to solve)
    if (data_nums < 100) {
        return;
    }

    hard_iron = []; //store hard iron biases
    soft_iron = [][]; //store soft iron biases 
    
    // Convert vector buffer to Eigen matrix for computation
    Eigen::MatrixXd data(data_nums, 3);
    for (int i = 0; i < data_nums; i++) {
        data(i, 0) = _magCalibBuffer[i].x();
        data(i, 1) = _magCalibBuffer[i].y();
        data(i, 2) = _magCalibBuffer[i].z();
    }
    
    // Extract individual columns for easier manipulation
    Eigen::MatrixXd data_x = data.col(0);
    Eigen::MatrixXd data_y = data.col(1);
    Eigen::MatrixXd data_z = data.col(2);
    
    // Build design matrix D with quadratic terms for ellipsoid fitting
    // Each row: [x², y², z², 2xy, 2xz, 2yz, 2x, 2y, 2z]
    Eigen::MatrixXd D(data_nums, 9);
    D.col(0) = data_x.array().square();              // x²
    D.col(1) = data_y.array().square();              // y²
    D.col(2) = data_z.array().square();              // z²
    D.col(3) = 2 * data_x.array() * data_y.array(); // 2xy
    D.col(4) = 2 * data_x.array() * data_z.array(); // 2xz
    D.col(5) = 2 * data_y.array() * data_z.array(); // 2yz
    D.col(6) = 2 * data_x.array();                   // 2x
    D.col(7) = 2 * data_y.array();                   // 2y
    D.col(8) = 2 * data_z.array();                   // 2z
    
    // Solve least squares: D'*D*v = D'*ones
    // This finds the ellipsoid parameters that best fit the data
    Eigen::MatrixXd A_v = D.transpose() * D;
    Eigen::MatrixXd b_v = D.transpose() * Eigen::MatrixXd::Ones(data_nums, 1);
    Eigen::MatrixXd x_v = A_v.lu().solve(b_v);
    
    // Form the algebraic representation of the ellipsoid as a 4x4 matrix
    // This represents the quadratic form: v'*A*v = 0
    Eigen::Matrix4d A;
    A << x_v(0), x_v(3), x_v(4), x_v(6),
         x_v(3), x_v(1), x_v(5), x_v(7),
         x_v(4), x_v(5), x_v(2), x_v(8),
         x_v(6), x_v(7), x_v(8), -1.0;
    
    // Extract the center of the ellipsoid (hard-iron offset)
    // Solve: -A[0:3,0:3] * center = [x_v(6), x_v(7), x_v(8)]
    Eigen::Matrix3d A_center = -A.block<3, 3>(0, 0);
    Eigen::Vector3d b_center(x_v(6), x_v(7), x_v(8));
    Eigen::Vector3d x_center = A_center.lu().solve(b_center);
    
    // Store hard-iron correction (offset to center the ellipsoid)
    hard_iron[0] = x_center(0);
    hard_iron[1] = x_center(1);
    hard_iron[2] = x_center(2);
    
    // Create translation matrix to move ellipsoid to origin
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<1, 3>(3, 0) = x_center.transpose();
    
    // Transform ellipsoid to centered position
    Eigen::Matrix4d R = T * A * T.transpose();
    
    // Perform eigenvalue decomposition on the centered ellipsoid
    // This gives us the principal axes and radii
    Eigen::EigenSolver<Eigen::Matrix3d> eig(R.block<3, 3>(0, 0) / (-R(3, 3)));
    Eigen::Matrix3d eigen_vectors = eig.pseudoEigenvectors();
    Eigen::Matrix3d eigen_values = eig.pseudoEigenvalueMatrix();
    
    // Ensure all eigenvalues are positive (flip eigenvector if needed)
    for (int i = 0; i < 3; i++) {
        if (eigen_values(i, i) < 0) {
            eigen_values(i, i) = -eigen_values(i, i);
            eigen_vectors.col(i) = -eigen_vectors.col(i);
        }
    }
    
    // Compute the radii of the ellipsoid from eigenvalues
    Eigen::Vector3d radii = (1.0 / eigen_values.diagonal().array()).sqrt();
    
    // Build soft-iron correction matrix
    // This transforms the ellipsoid into a sphere with radius = smallest radius
    Eigen::Matrix3d map = eigen_vectors.transpose();       // Rotation to principal axes
    Eigen::Matrix3d inv_map = eigen_vectors;               // Rotation back
    
    // Scale matrix to normalize radii
    Eigen::Matrix3d scale_matrix = Eigen::Matrix3d::Identity();
    scale_matrix(0, 0) = radii(0);
    scale_matrix(1, 1) = radii(1);
    scale_matrix(2, 2) = radii(2);
    
    // Normalize to smallest radius to preserve magnitude
    Eigen::Matrix3d scale = scale_matrix.inverse() * radii.minCoeff();
    
    // Final soft-iron correction: rotate -> scale -> rotate back
    Eigen::Matrix3d soft_corr_matrix = inv_map * scale * map;
    
    // Store soft-iron correction matrix
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            soft_iron[i][j] = soft_corr_matrix(i, j);
        }
    }
    
    // Mark calibration as complete
    _magCalibrated = true;

    
    // Apply calibration corrections to the accumulated _sumMag vector
    // Step 1: Apply hard-iron offset
    Vector<3> temp;
    temp.x() = _sumMag.x() - hard_iron[0];
    temp.y() = _sumMag.y() - hard_iron[1];
    temp.z() = _sumMag.z() - hard_iron[2];
    
    // Step 2: Apply soft-iron correction and update _sumMag
    _sumMag.x() = soft_iron[0][0] * temp.x() + soft_iron[0][1] * temp.y() + soft_iron[0][2] * temp.z();
    _sumMag.y() = soft_iron[1][0] * temp.x() + soft_iron[1][1] * temp.y() + soft_iron[1][2] * temp.z();
    _sumMag.z() = soft_iron[2][0] * temp.x() + soft_iron[2][1] * temp.y() + soft_iron[2][2] * temp.z();
    _bufferClear() //clear buffer
}

// Apply calibration to a single raw magnetometer reading
// Returns calibrated measurement with hard and soft iron corrections applied
//this function is called every time in order to apply corrections to mag 
Vector<3> calibrateMag(const Vector<3> &raw) {
    // If not calibrated, return raw reading
    if (!_magCalibrated) {
        return raw;
    }
    
    // Step 1: Apply hard-iron offset (subtract center of ellipsoid)
    Vector<3> temp;
    temp.x() = raw.x() - hard_iron[0];
    temp.y() = raw.y() - hard_iron[1];
    temp.z() = raw.z() - hard_iron[2];
    
    // Step 2: Apply soft-iron correction (transform ellipsoid to sphere)
    Vector<3> corrected;
    corrected.x() = soft_iron[0][0] * temp.x() + soft_iron[0][1] * temp.y() + soft_iron[0][2] * temp.z();
    corrected.y() = soft_iron[1][0] * temp.x() + soft_iron[1][1] * temp.y() + soft_iron[1][2] * temp.z();
    corrected.z() = soft_iron[2][0] * temp.x() + soft_iron[2][1] * temp.y() + soft_iron[2][2] * temp.z();
    
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
    vector<Vector<3>> _magCalibBuffer; //temporary buffer to hold mag samples 
    bool _magCalibrated = false;

};

#endif // MAHONY_H