#ifndef LINEARKALMANFILTER_H
#define LINEARKALMANFILTER_H

#include "MatrixMult.h"
//#include "RFM69Helper.h"
#include <Arduino.h>

typedef struct
{
    double *state_transition_matrix;
    double *control_matrix;
    double *observation_matrix;
    double *measurement_covariance;
    double *process_noise_covariance;
    double *kalman_gain;
    double *current_state;
    double *current_covariance;
    double *current_input;
    double *current_measurement;
    int state_size;
    int input_size;
    int measurement_size;
    double last_time;
    double current_time;
} KFState;

void copy_array(double *from, double *to, int len);

double get_time_elapsed(KFState *state);

void init(KFState *state, int state_size, int input_size, int measurement_size,
          double *initial_state, double *initial_input, double *initial_covariance,
          double *measurement_covariance, double *process_noise_covariance);

void update_input(KFState *state, double *input);

void update_measurement(KFState *state, double *measurement);

void get_state_transition_matrix(KFState *state);

void get_control_matrix(KFState *state);

void get_observation_matrix(KFState *state, int has_gps, int has_barometer);

void covariance_extrapolation(KFState *state);

void covariance_update(KFState *state);

void kalman_update(KFState *state);

void state_extrapolation(KFState *state);

void state_update(KFState *state);

void predict(KFState *state, int has_imu, double *input);

void update(KFState *state, int has_gps, int has_barometer, double *measurement);

double *get_state(KFState *state);

double *get_inputs(KFState *state);

double get_last_time(KFState *state);

void updateFilter(KFState *state, double time, int has_gps, int has_barometer, int has_imu, double *measurements, double *inputs, double **predictions);

#endif