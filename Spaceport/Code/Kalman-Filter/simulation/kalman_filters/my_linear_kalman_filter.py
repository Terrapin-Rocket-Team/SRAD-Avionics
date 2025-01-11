from .base_linear_kalman_filter import BaseLinearKalmanFilter
import numpy as np
from typing import Optional

class MyLinearKalmanFilter(BaseLinearKalmanFilter):
    """
    A concrete implementation of a linear Kalman Filter 
    that sets up the F, G, H, Q, R matrices for a 6x6 state, etc.
    """

    def __init__(self, 
                 initial_state: np.ndarray,
                 initial_covariance: np.ndarray,
                 control_input: np.ndarray,
                 measurement_noise: np.ndarray,
                 process_noise: np.ndarray):
        
        super().__init__(initial_state, initial_covariance, control_input)
        self.R = measurement_noise
        self.Q = process_noise

    def iterate(self, dt, measurement: np.ndarray, control: np.ndarray, true_state: Optional[np.ndarray] = None):
        """
        One cycle of predict -> update.

        :param dt: Time step
        :param measurement: Measurement vector (3x1)
        :param control: Control vector (3x1)
        :param true_state: True state vector (6x1) for NEES calculation (only for generated data)
        """
        # Update control vector
        self.u = control

        # Build your F, G, H, Q, R as needed each iteration
        self._build_matrices(dt)

        # Predict
        self.predict_state()
        self.covariance_extrapolate()

        # update metrics
        y = measurement - (self.H @ self.x)  # Innovation
        S = self.H @ self.P @ self.H.T + self.R  # Innovation covariance
        self.innovations.append(y.flatten())
        self.S_matrices.append(S)
        self.P_matrices.append(self.P.copy())

        # Update (since we have the new measurement)
        self.calculate_kalman_gain()
        self.update_with_measurement(measurement)
        self.covariance_update()

        # if we have true state, calculate NEES
        if true_state is not None:
            error = self.x - true_state
            self.estimation_errors.append(error.flatten())

        return self

    def _build_matrices(self, dt):
        """
        Build the specific F, G, Q, R, H for your system 
        (6D state: position + velocity).
        """
        self.F = np.array([
            [1, 0, 0, dt, 0,  0],
            [0, 1, 0, 0,  dt, 0],
            [0, 0, 1, 0,  0,  dt],
            [0, 0, 0, 1,  0,  0],
            [0, 0, 0, 0,  1,  0],
            [0, 0, 0, 0,  0,  1]
        ])

        self.G = np.array([
            [0.5*dt**2, 0,        0],
            [0,         0.5*dt**2,0],
            [0,         0,        0.5*dt**2],
            [dt,        0,        0],
            [0,         dt,       0],
            [0,         0,        dt]
        ])

        self.H = np.array([
            [1, 0, 0, 0, 0, 0],  # Observing x
            [0, 1, 0, 0, 0, 0],  # Observing y
            [0, 0, 1, 0, 0, 0]   # Observing z
        ])

