from .base_kalman_filter import BaseKalmanFilter
import abc
import numpy as np

class BaseExtendedKalmanFilter(BaseKalmanFilter):
    """
    Abstract class for an Extended Kalman Filter (EKF).
    This implements the general EKF prediction and update steps but requires
    system-specific models (state transition, measurement function, and Jacobians)
    to be defined by subclasses.
    """

    def __init__(self,  initial_state: np.ndarray, initial_covariance: np.ndarray):
        """
        :param initial_state: Nx1 state vector
        :param initial_covariance: NxN covariance matrix
        """
        self.num_states = len(initial_state) # calculates the amount of states
        self.x = initial_state
        self.P = initial_covariance

        self.F = None  # Jacobian of State Prediction Function (State Transition Function)
        self.H = None  # Jacobian of Measurement Function
        self.Q = None  # Process noise
        self.R = None  # Measurement noise
        self.K = None  # Kalman gain
      
        # Lists to store metrics
        self.innovations = []
        self.S_matrices = []            # Innovation covariance
        self.estimation_errors = []
        self.P_matrices = []            # Covariance matrices

    def predict_state(self, dt):
        """EKF prediction step."""
        F = self.compute_jacobian_F(self.x, dt)  # Compute Jacobian of f
        self.x = self.state_transition_function(self.x, dt)  # Apply nonlinear transition
        self.P = F @ self.P @ F.T + self.Q  # Covariance update

    def update_with_measurement(self, measurement):
        """EKF update step using nonlinear measurement model."""
        H = self.compute_jacobian_H(self.x)  # Compute Jacobian of h
        z_pred = self.measurement_function(self.x)  # Predicted measurement
        y = measurement - z_pred  # Residual
        S = H @ self.P @ H.T + self.R  # Innovation covariance
        K = self.P @ H.T @ np.linalg.inv(S)  # Kalman gain

        self.x += K @ y  # Update state estimate
        self.P = (np.eye(self.num_states) - K @ H) @ self.P  # Update covariance

    def get_state(self):
        return self.x
    
    def set_parameters(self, **kwargs):
        """Update filter parameters like process noise or measurement noise."""
        if "process_noise" in kwargs:
            self.Q = kwargs["process_noise"]
        if "measurement_noise" in kwargs:
            self.R = kwargs["measurement_noise"]

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
    
    @abc.abstractmethod
    def state_transition_function(self, state, dt):
        """Defines the nonlinear state transition model f(x, u, dt)."""
        pass

    @abc.abstractmethod
    def compute_jacobian_F(self, state, dt):
        """Computes the Jacobian of f(x, u, dt)."""
        pass

    @abc.abstractmethod
    def measurement_function(self, state):
        """Defines the nonlinear measurement model h(x)."""
        pass

    @abc.abstractmethod
    def compute_jacobian_H(self, state):
        """Computes the Jacobian of h(x)."""
        pass