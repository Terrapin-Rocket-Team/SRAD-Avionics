#include "AbhiKalmanFilter.h"
namespace akf
{
    void init(KFState *state, int state_size, int input_size, int measurement_size,
              double *initial_state, double *initial_input, double *initial_covariance,
              double *measurement_covariance, double *process_noise_covariance)
    {
        state->state_size = state_size;
        state->input_size = input_size;
        state->measurement_size = measurement_size;
        state->current_measurement = new double[state->measurement_size];
        state->current_state = new double[state->state_size];
        state->state_transition_matrix = new double[state->state_size * state->state_size];
        state->control_matrix = new double[state->state_size * state->input_size];
        state->observation_matrix = new double[state->measurement_size * state->state_size];
        copy_array(initial_state, state->current_state, state->state_size);
        state->current_input = new double[state->input_size];
        copy_array(initial_input, state->current_input, state->input_size);
        state->current_covariance = new double[state->state_size * state->state_size];
        copy_array(initial_covariance, state->current_covariance, state->state_size * state->state_size);
        state->measurement_covariance = new double[state->measurement_size * state->measurement_size];
        copy_array(measurement_covariance, state->measurement_covariance, state->measurement_size * state->measurement_size);
        state->process_noise_covariance = new double[state->state_size * state->state_size];
        copy_array(process_noise_covariance, state->process_noise_covariance, state->state_size * state->state_size);
        state->last_time = 0;
        state->current_time = 0;
    }

    void copy_array(double *from, double *to, int len)
    {
        for (int i = 0; i < len; i++)
        {
            to[i] = from[i];
        }
    }

    double get_time_elapsed(KFState *state)
    {
        return state->current_time - state->last_time;
    }

    void update_input(KFState *state, double *input)
    {
        copy_array(input, state->current_input, state->input_size);
    }

    void update_measurement(KFState *state, double *measurement)
    {
        copy_array(measurement, state->current_measurement, state->measurement_size);
    }

    void get_state_transition_matrix(KFState *state)
    {
        double t = get_time_elapsed(state);
        double *new_matrix = new double[state->state_size * state->state_size]{1, 0, 0, t, 0, 0,
                                                                               0, 1, 0, 0, t, 0,
                                                                               0, 0, 1, 0, 0, t,
                                                                               0, 0, 0, 1, 0, 0,
                                                                               0, 0, 0, 0, 1, 0,
                                                                               0, 0, 0, 0, 0, 1};
        copy_array(new_matrix, state->state_transition_matrix, state->state_size * state->state_size);
        delete[] new_matrix;
    }

    void get_control_matrix(KFState *state)
    {
        double t = get_time_elapsed(state);
        double *new_matrix = new double[state->state_size * state->input_size]{0.5 * t * t, 0, 0,
                                                                               0, 0.5 * t * t, 0,
                                                                               0, 0, 0.5 * t * t,
                                                                               t, 0, 0,
                                                                               0, t, 0,
                                                                               0, 0, t};
        copy_array(new_matrix, state->control_matrix, state->state_size * state->input_size);
        delete[] new_matrix;
    }

    void get_observation_matrix(KFState *state, int has_gps, int has_barometer)
    {
        int sum = (has_barometer + has_gps) == 0 ? 1 : (has_barometer + has_gps);


        double *new_matrix;
        new_matrix = new double[state->measurement_size * state->state_size]{1.0 * has_gps, 0, 0, 0, 0, 0,
                                                                             0, 1.0 * has_gps, 0, 0, 0, 0,
                                                                             0, 0, 1.0/sum * has_gps, 0, 0, 0,
                                                                             0, 0, 1.0/sum * has_barometer, 0, 0, 0};
        copy_array(new_matrix, state->observation_matrix, state->measurement_size * state->state_size);
        delete[] new_matrix;
    }

    void covariance_extrapolation(KFState *state)
    {
        state->current_covariance =
            addMatrices(
                multiplyMatrices(
                    multiplyMatrices(
                        state->state_transition_matrix,
                        state->current_covariance,
                        state->state_size, state->state_size, state->state_size, state->state_size),
                    transposeMatrix(
                        state->state_transition_matrix,
                        state->state_size, state->state_size),
                    state->state_size, state->state_size, state->state_size, state->state_size),
                state->process_noise_covariance,
                state->state_size, state->state_size);
    }

    void covariance_update(KFState *state)
    {
        double *identity_matrix = ident(state->state_size);
        state->current_covariance =
            addMatrices(
                multiplyMatrices(
                    multiplyMatrices(
                        subMatrices(
                            identity_matrix,
                            multiplyMatrices(
                                state->kalman_gain,
                                state->observation_matrix,
                                state->state_size, state->measurement_size, state->measurement_size, state->state_size),
                            state->state_size, state->state_size),
                        state->current_covariance,
                        state->state_size, state->state_size, state->state_size, state->state_size),
                    transposeMatrix(
                        subMatrices(
                            identity_matrix,
                            multiplyMatrices(
                                state->kalman_gain,
                                state->observation_matrix,
                                state->state_size, state->measurement_size, state->measurement_size, state->state_size),
                            state->state_size, state->state_size),
                        state->state_size, state->state_size),
                    state->state_size, state->state_size, state->state_size, state->state_size),
                multiplyMatrices(
                    multiplyMatrices(
                        state->kalman_gain,
                        state->measurement_covariance,
                        state->state_size, state->measurement_size, state->measurement_size, state->measurement_size),
                    transposeMatrix(
                        state->kalman_gain,
                        state->state_size, state->measurement_size),
                    state->state_size, state->measurement_size, state->measurement_size, state->state_size),
                state->state_size, state->state_size);
    }

    void kalman_update(KFState *state)
    {
        state->kalman_gain =
            multiplyMatrices(
                multiplyMatrices(
                    state->current_covariance,
                    transposeMatrix(
                        state->observation_matrix,
                        state->measurement_size, state->state_size),
                    state->state_size, state->state_size, state->state_size, state->measurement_size),
                inverseMatrix(
                    addMatrices(
                        multiplyMatrices(
                            multiplyMatrices(
                                state->observation_matrix,
                                state->current_covariance,
                                state->measurement_size, state->state_size, state->state_size, state->state_size),
                            transposeMatrix(
                                state->observation_matrix,
                                state->measurement_size, state->state_size),
                            state->measurement_size, state->state_size, state->state_size, state->measurement_size),
                        state->measurement_covariance,
                        state->measurement_size, state->measurement_size),
                    state->measurement_size),
                state->state_size, state->measurement_size, state->measurement_size, state->measurement_size);
    }

    void state_extrapolation(KFState *state)
    {
        state->current_state =
            addMatrices(
                multiplyMatrices(
                    state->state_transition_matrix,
                    state->current_state,
                    state->state_size, state->state_size, state->state_size, 1),
                multiplyMatrices(
                    state->control_matrix,
                    state->current_input,
                    state->state_size, state->input_size, state->input_size, 1),
                state->state_size, 1);
    }

    void state_update(KFState *state)
    {
        state->current_state =
            addMatrices(
                state->current_state,
                multiplyMatrices(
                    state->kalman_gain,
                    subMatrices(
                        state->current_measurement,
                        multiplyMatrices(
                            state->observation_matrix,
                            state->current_state,
                            state->measurement_size, state->state_size, state->state_size, 1),
                        state->measurement_size, 1),
                    state->state_size, state->measurement_size, state->measurement_size, 1),
                state->state_size, 1);
    }

    void predict(KFState *state, double *input)
    {
        get_state_transition_matrix(state);
        get_control_matrix(state);
        update_input(state, input);
        state_extrapolation(state);
        covariance_extrapolation(state);
        kalman_update(state);
    }

    void update(KFState *state, int has_gps, int has_barometer, double *measurement)
    {
        update_measurement(state, measurement);
        get_observation_matrix(state, has_gps, has_barometer);
        state_update(state);
        covariance_update(state);
    }

    double *get_state(KFState *state)
    {
        return state->current_state;
    }

    double *get_inputs(KFState *state)
    {
        return state->current_input;
    }

    double get_last_time(KFState *state)
    {
        return state->current_time;
    }

    void updateFilter(KFState *state, double time, int has_gps, int has_barometer, int has_imu, double *measurements, double *inputs, double **predictions)
    {
        state->last_time = state->current_time;
        state->current_time = time;
        predict(state, inputs);
        update(state, has_gps, has_barometer, measurements);
        (*predictions)[0] = get_last_time(state);
        double *return_state = get_state(state);
        for (int i = 0; i < 6; i++)
        {
            (*predictions)[i + 1] = return_state[i];
        }
        double *return_inputs = get_inputs(state);
        for (int i = 0; i < 3; i++)
        {
            (*predictions)[i + 6] = return_inputs[i];
        }
    }
}