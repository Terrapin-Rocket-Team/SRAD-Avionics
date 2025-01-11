# evaluation/metrics.py
import numpy as np

def compute_nis(residuals, S):
    """
    Normalized Innovation Squared (NIS).
    residuals: z_meas - H*x_est, shape (3,)
    S: measurement covariance, shape (3x3)
    return: scalar
    """
    return residuals.T @ np.linalg.inv(S) @ residuals  # shape: (1,1)

def compute_nees(state_error, P):
    """
    Normalized Estimation Error Squared (NEES).
    state_error: x_true - x_est, shape (6,)
    P: estimate covariance, shape (6x6)
    return: scalar
    """
    return state_error.T @ np.linalg.inv(P) @ state_error

def compute_regularization(params):
    """
    Simple L2 norm or other measure of param magnitude to prevent overfitting.
    """
    return 1e-3 * np.sum(params**2)

