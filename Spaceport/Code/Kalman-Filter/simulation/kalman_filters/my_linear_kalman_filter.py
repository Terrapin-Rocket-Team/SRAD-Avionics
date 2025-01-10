from .base_linear_kalman_filter import BaseLinearKalmanFilter
import numpy as np

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
                 process_noise=5.0):
        
        super().__init__(initial_state, initial_covariance, control_input)
        self.process_noise = process_noise
        self.R = measurement_noise

    def iterate(self, dt, measurement: np.ndarray, control: np.ndarray):
        """
        One cycle of predict -> update.
        """
        # Update control vector
        self.u = control

        # Build your F, G, H, Q, R as needed each iteration
        self._build_matrices(dt)

        # Update (since we have the new measurement)
        self.calculate_kalman_gain()
        self.update_with_measurement(measurement)
        self.covariance_update()

        # Predict
        self.predict_state()
        self.covariance_extrapolate()

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

        # Q = G * process_noise^2 * G^T
        self.Q = (self.process_noise**2) * (self.G @ self.G.T)
