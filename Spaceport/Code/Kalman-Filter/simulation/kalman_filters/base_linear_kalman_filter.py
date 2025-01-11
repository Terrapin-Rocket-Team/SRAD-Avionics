from .base_kalman_filter import BaseKalmanFilter
import numpy as np

class BaseLinearKalmanFilter(BaseKalmanFilter):
    """
    A more specific abstract class for linear Kalman Filters, 
    adding typical linear KF matrix structures.
    """

    def __init__(self, 
                 initial_state: np.ndarray,
                 initial_covariance: np.ndarray,
                 control_input: np.ndarray):
        """
        :param initial_state: Nx1 state vector
        :param initial_covariance: NxN covariance matrix
        :param control_input: Lx1 control vector
        """
        self.x = initial_state
        self.P = initial_covariance
        self.u = control_input
        # Matrices typically used in linear KFs
        self.F = None  # State transition
        self.G = None  # Control matrix
        self.H = None  # Observation matrix
        self.K = None  # Kalman gain
        self.Q = None  # Process noise
        self.R = None  # Measurement noise

        # Lists to store metrics
        self.innovations = []
        self.S_matrices = []            # Innovation covariance
        self.estimation_errors = []
        self.P_matrices = []            # Covariance matrices

    def get_state(self):
        return self.x

    def set_parameters(self, **kwargs):
        """
        Allows external objects to override parameters, e.g.:
        - process_noise
        - measurement_noise
        - etc.
        """
        for k, v in kwargs.items():
            setattr(self, k, v)

    def predict_state(self):
        # x = F*x + G*u
        self.x = self.F @ self.x + self.G @ self.u
        return self.x

    def update_with_measurement(self, measurement: np.ndarray):
        # x = x + K * (z - Hx)
        self.x = self.x + self.K @ (measurement - (self.H @ self.x))

    def covariance_extrapolate(self):
        # P = F * P * F^T + Q
        self.P = self.F @ self.P @ self.F.T + self.Q

    def covariance_update(self):
        # P = (I - K*H)*P*(I - K*H)^T + K*R*K^T
        I = np.eye(self.P.shape[0])
        self.P = (I - self.K @ self.H) @ self.P @ (I - self.K @ self.H).T + self.K @ self.R @ self.K.T

    def calculate_kalman_gain(self):
        # K = P * H^T * inv(H * P * H^T + R)
        S = self.H @ self.P @ self.H.T + self.R
        self.K = self.P @ self.H.T @ np.linalg.inv(S)
        return self.K
    
    def reset_metrics(self):
        """Reset stored metrics."""
        self.innovations = []
        self.S_matrices = []
        self.estimation_errors = []
        self.P_matrices = []

    def get_metrics(self):
        """Retrieve stored metrics."""
        return {
            "innovations": np.array(self.innovations),
            "S_matrices": np.array(self.S_matrices),
            "estimation_errors": np.array(self.estimation_errors),
            "P_matrices": np.array(self.P_matrices)
        }
