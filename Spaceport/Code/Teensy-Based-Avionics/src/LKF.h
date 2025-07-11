#ifndef LKF_H
#define LKF_H

#include "Math/Matrix.h"
#include "Math/Vector.h"

using namespace mmfs;

static const int n = 6, m = 3;
class LKF
{
public:
    LKF(
        const Matrix &H_in,
        const Matrix &R_in,
        double sigma_accel)
        : H(H_in),
          R(R_in),
          sigma_acc(sigma_accel)
    {
        x = Matrix(n, 1, new double[n * 1]); // state vector
        for (int i = 0; i < n; i++)
            x.getArr()[i] = 0.0;
        P = Matrix::ident(n) * 10; // initial uncertainty
    }

    // Call this every time you get (u, z, dt)
    void update(const Vector<3> &u, const Vector<3> &z, double dt)
    {
        Matrix u_m(3, 1, new double[3]{u.x(), u.y(), u.z()});
        Matrix z_m(3, 1, new double[3]{z.x(), z.y(), z.z()});
        // --- build per‐step matrices ---
        build_F_B_Q(dt);
        // --- predict ---
        x_pred = F * x + B * u_m;
        P_pred = F * P * F.transpose() + Q;
        // --- update ---
        y = z_m - H * x_pred;
        S = H * P_pred * H.transpose() + R;
        K = P_pred * H.transpose() * S.inverse();
        x = x_pred + K * y;
        P = (Matrix::ident(n) - K * H) * P_pred;
    }

    Vector<n> state() const
    {
        Vector<n> v;
        for (int i = 0; i < n; i++)
            v[i] = x.getArr()[i];
        return v;
    }
    Matrix covariance() const { return P; }

private:
    double sigma_acc;

    Matrix x;    // persistent
    Matrix P;    // persistent
    Matrix H, R; // persistent

    // buffers for per‐step work
    Matrix F, B, Q, x_pred, P_pred, S, K;
    Matrix y;

    void build_F_B_Q(double dt)
    {
        F = Matrix::ident(n);
        for (int i = 0; i < 3; ++i)
            F(i, i + 3) = dt;

        B = Matrix(n, 3, new double[n * 3]);
        // position rows
        for (int i = 0; i < 3; ++i)
            B(i, i) = 0.5 * dt * dt;
        // velocity rows
        for (int i = 0; i < 3; ++i)
            B(i + 3, i) = dt;

        // Q = B * (σ_acc² * I₃) * Bᵀ
        Matrix Q_base = Matrix::ident(3) * (sigma_acc * sigma_acc);
        Q = B * Q_base * B.transpose();
    }
};

#endif // LKF_H