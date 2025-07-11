#ifndef LKF_H
#define LKF_H

#include "Math/Matrix.h"
#include "Math/Vector.h"

using namespace mmfs;

static const int N = 9, M = 3;
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
        x = Matrix(N, 1, new double[N * 1]); // state vector
        for (int i = 0; i < N; i++)
            x.getArr()[i] = 0.0;
        P = Matrix::ident(N) * 10; // initial uncertainty
    }

    // Call this every time you get (u, z, dt)
    void update(const Vector<3> &u, const Vector<3> &z, double dt)
    {
        Matrix u_m(3, 1, new double[3]{u.x(), u.y(), u.z()});
        Matrix z_m(3, 1, new double[3]{z.x(), z.y(), z.z()});
        // --- build per‐step matrices ---
        build_F_B_Q(dt);

        // pull bias out of the state vector
        double bx = x.getArr()[6];
        double by = x.getArr()[7];
        double bz = x.getArr()[8];

        // form an “unbiased” accel control
        Matrix u_eff(3, 1, new double[3]{u.x() - bx, u.y() - by, u.z() - bz});
        
        // --- predict ---
        x_pred = _F * x + B * u_eff;
        P_pred = _F * P * _F.transpose() + Q;
        // --- update ---
        y = z_m - H * x_pred;
        S = H * P_pred * H.transpose() + R;
        K = P_pred * H.transpose() * S.inverse();
        x = x_pred + K * y;
        P = (Matrix::ident(N) - K * H) * P_pred;
    }

    Vector<N> state() const
    {
        Vector<N> v;
        for (int i = 0; i < N; i++)
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
    Matrix _F, B, Q, x_pred, P_pred, S, K;
    Matrix y;

    void build_F_B_Q(double dt)
    {
        _F = Matrix::ident(N);
        double half_dt2 = 0.5 * dt * dt;
        for (int i = 0; i < 3; ++i)
        {
            // position ← position + velocity·dt + (u_i - b_i)*0.5·dt²
            _F(i, i + 3) = dt;        // p  depends on v
            _F(i, 6 + i) = -half_dt2; // p  depends on -b
            // velocity ← velocity + (u_i - b_i)·dt
            _F(i + 3, 6 + i) = -dt; // v  depends on -b
        }

        B = Matrix(N, 3, new double[N * 3]);
        for( int i = 0; i < N * 3; i++)
            B.getArr()[i] = 0.0;

        //   → position rows get 0.5·dt²
        //   → velocity rows get dt
        for (int i = 0; i < 3; ++i)
        {
            B(i, i) = half_dt2;
            B(i + 3, i) = dt;
        }

        // Q = B * (σ_acc² * I₃) * Bᵀ
        Matrix Q_base = Matrix::ident(3) * (sigma_acc * sigma_acc);
        Q = B * Q_base * B.transpose();

        double sigma_b = 0.05; // e.g. ~0.01 m/s² drift per step
        for (int i = 0; i < 3; ++i)
        {
            Q(6 + i, 6 + i) = sigma_b * sigma_b;
        }
    }
};

#endif // LKF_H