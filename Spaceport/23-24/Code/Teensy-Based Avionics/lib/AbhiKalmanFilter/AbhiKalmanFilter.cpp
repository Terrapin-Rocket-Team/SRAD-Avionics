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
                                                                             0, 0, 1.0 / sum * has_gps, 0, 0, 0,
                                                                             0, 0, 1.0 / sum * has_barometer, 0, 0, 0};
        copy_array(new_matrix, state->observation_matrix, state->measurement_size * state->state_size);
        delete[] new_matrix;
    }

    void covariance_extrapolation(KFState *state)
    {
        delete[] state->current_covariance;
        double *m1 =
            multiplyMatrices(
                state->state_transition_matrix,
                state->current_covariance,
                state->state_size, state->state_size, state->state_size, state->state_size);
        double *t1 =
            transposeMatrix(
                state->state_transition_matrix,
                state->state_size, state->state_size);
        double *m2 =
            multiplyMatrices(
                m1,
                t1,
                state->state_size, state->state_size, state->state_size, state->state_size);
        state->current_covariance =
            addMatrices(
                m2,
                state->process_noise_covariance,
                state->state_size, state->state_size);
        delete[] m1;
        delete[] t1;
        delete[] m2;
    }

    void covariance_update(KFState *state)
    {
        delete[] state->current_covariance;
        double *identity_matrix = ident(state->state_size);
        double *m1 =
            multiplyMatrices(
                state->kalman_gain,
                state->observation_matrix,
                state->state_size, state->measurement_size, state->measurement_size, state->state_size);
        double *s1 =
            subMatrices(
                identity_matrix,
                m1,
                state->state_size, state->state_size);
        double *m2 =
            multiplyMatrices(
                s1,
                state->current_covariance,
                state->state_size, state->state_size, state->state_size, state->state_size);
        double *m3 =
            multiplyMatrices(
                state->kalman_gain,
                state->observation_matrix,
                state->state_size, state->measurement_size, state->measurement_size, state->state_size);
        double *s2 =
            subMatrices(
                identity_matrix,
                m3,
                state->state_size, state->state_size);
        double *t1 =
            transposeMatrix(
                s2,
                state->state_size, state->state_size);
        double *m4 =
            multiplyMatrices(
                m2,
                t1,
                state->state_size, state->state_size, state->state_size, state->state_size);
        double *m5 =
            multiplyMatrices(
                state->kalman_gain,
                state->measurement_covariance,
                state->state_size, state->measurement_size, state->measurement_size, state->measurement_size);
        double *t2 =
            transposeMatrix(
                state->kalman_gain,
                state->state_size, state->measurement_size);
        double *m6 =
            multiplyMatrices(
                m5,
                t2,
                state->state_size, state->measurement_size, state->measurement_size, state->state_size);

        state->current_covariance =
            addMatrices(
                m4,
                m6,
                state->state_size, state->state_size);
        delete[] identity_matrix;
        delete[] m1;
        delete[] s1;
        delete[] m2;
        delete[] m3;
        delete[] s2;
        delete[] t1;
        delete[] m4;
        delete[] m5;
        delete[] t2;
        delete[] m6;
    }

    void kalman_update(KFState *state)
    {
        delete[] state->kalman_gain;
        double *t1 =
            transposeMatrix(
                state->observation_matrix,
                state->measurement_size, state->state_size);
        double *m1 =
            multiplyMatrices(
                state->current_covariance,
                t1,
                state->state_size, state->state_size, state->state_size, state->measurement_size);
        double *m2 =
            multiplyMatrices(
                state->observation_matrix,
                state->current_covariance,
                state->measurement_size, state->state_size, state->state_size, state->state_size);
        double *m3 =
            multiplyMatrices(
                m2,
                t1,
                state->measurement_size, state->state_size, state->state_size, state->measurement_size);
        double *a1 =
            addMatrices(
                m3,
                state->measurement_covariance,
                state->measurement_size, state->measurement_size);
        double *i1 =
            inverseMatrix(
                a1,
                state->measurement_size);

        state->kalman_gain =
            multiplyMatrices(m1,
                             i1,
                             state->state_size, state->measurement_size, state->measurement_size, state->measurement_size);
        delete[] t1;
        delete[] m1;
        delete[] m2;
        delete[] m3;
        delete[] a1;
        delete[] i1;
    }

    void state_extrapolation(KFState *state)
    {
        delete[] state->current_state;
        double *m1 =
            multiplyMatrices(
                state->state_transition_matrix,
                state->current_state,
                state->state_size, state->state_size, state->state_size, 1);
        double *m2 =
            multiplyMatrices(
                state->control_matrix,
                state->current_input,
                state->state_size, state->input_size, state->input_size, 1);
        state->current_state =
            addMatrices(
                m1,
                m2,
                state->state_size, 1);
        delete[] m1;
        delete[] m2;
    }

    void state_update(KFState *state)
    {
        delete[] state->current_state;
        double *m1 =
            multiplyMatrices(
                state->observation_matrix,
                state->current_state,
                state->measurement_size, state->state_size, state->state_size, 1);
        double *s1 =
            subMatrices(
                state->current_measurement,
                m1,
                state->measurement_size, 1);
        double *m2 =
            multiplyMatrices(
                state->kalman_gain,
                s1,
                state->state_size, state->measurement_size, state->measurement_size, 1);
        state->current_state =
            addMatrices(
                state->current_state,
                m2,
                state->state_size, 1);
        delete[] m1;
        delete[] s1;
        delete[] m2;
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