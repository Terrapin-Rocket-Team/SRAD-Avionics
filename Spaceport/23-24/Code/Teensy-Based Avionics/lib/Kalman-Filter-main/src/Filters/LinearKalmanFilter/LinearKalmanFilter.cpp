#include "LinearKalmanFilter.h"
#include "../../MatrixMult/Matrix.h"

LinearKalmanFilter::LinearKalmanFilter(Matrix X, Matrix U, Matrix P, Matrix F, Matrix G, Matrix R){
    state.X = X;
    state.U = U;
    state.P = P;
    state.F = F;
    state.G = G;
    state.R = R;
    calculate_initial_values();
}

void LinearKalmanFilter::predict_state(){
    state.X = (state.F * state.X) + (state.G * state.U);
}

void LinearKalmanFilter::estimate_state(Matrix measurement){
    state.X = state.X + state.K*(measurement - state.H*state.X);
}

void LinearKalmanFilter::calculate_kalman_gain(){
    state.K = state.P * state.H.T() * (state.H * state.P * state.H.T() + state.R).inv();
}

void LinearKalmanFilter::covariance_update(){
    int n = state.X.getRows();
    state.P = ((Matrix::ident(n) - state.K*state.H)*state.P*((Matrix::ident(n) - state.K*state.H).T())) + state.K*state.R*state.K.T();
}

void LinearKalmanFilter::covariance_extrapolate(){
    state.P = state.F*state.P*state.F.T() + state.Q;
}

void LinearKalmanFilter::calculate_initial_values(){    
    state.Q = (state.G*0.2*0.2)*state.G.T();
    predict_state();
    covariance_extrapolate();
}

Matrix LinearKalmanFilter::iterate(Matrix measurement, Matrix control, Matrix F, Matrix G, Matrix H){
    state.F = F;
    state.G = G;
    state.H = H;
    state.U = control;
    state.Q = (state.G*1.2*1.2)*state.G.T();
    calculate_kalman_gain();
    estimate_state(measurement);
    covariance_update();
    predict_state();
    covariance_extrapolate();
    return state.X;
}