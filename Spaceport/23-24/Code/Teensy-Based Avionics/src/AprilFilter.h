#ifndef APRILFILTER_H
#define APRILFILTER_H

#include "Kalman_Filter.h"

Matrix get_F(int dt);
Matrix get_G(int dt);
Matrix get_H(int has_gps, int has_barometer);
LinearKalmanFilter *initializeFilter();
double* iterateFilter(LinearKalmanFilter kf, int dt, double* input, double* measurement, int has_gps, int has_barometer);


#endif