#ifndef AVIONICS_KF_H
#define AVIONICS_KF_H

#include "../src/Filters/LinearKalmanFilter.h"

namespace astra {

class AvionicsKF : public LinearKalmanFilter {
public:
    AvionicsKF();
    ~AvionicsKF() = default;

    // Override getter methods to provide subteam-specific matrix implementations
    void initialize() override {};
    Matrix getF(double dt) override;
    Matrix getG(double dt) override;
    Matrix getH() override;
    Matrix getR() override;
    Matrix getQ(double dt) override;
};

} // namespace astra

#endif // AVIONICS_KF_H
