"""
utility functions
"""

import numpy as np

# optimization/parameter_space.py
import numpy as np

class ParameterSpace:
    def __init__(self, 
                 p0_lower_dim=21,  # 6x6 lower triangle
                 r_lower_dim=6,    # 3x3 lower triangle
                 q_lower_dim=21,   # optional 6x6 process noise
                 # or maybe a single scalar for process noise, 
                 # set q_lower_dim=1 if it's just a scalar
                 p0_bounds=None,
                 r_bounds=None,
                 q_bounds=None):
        """
        p0_bounds, r_bounds, q_bounds: lists of (low, high) for each dimension 
        or None for default.
        """
        self.p0_lower_dim = p0_lower_dim
        self.r_lower_dim = r_lower_dim
        self.q_lower_dim = q_lower_dim

        # For each dimension, define (min, max). 
        # If None, define some default.
        self.p0_bounds = p0_bounds if p0_bounds else [(-10, 10)]*p0_lower_dim
        self.r_bounds = r_bounds if r_bounds else [(-10, 10)]*r_lower_dim
        self.q_bounds = q_bounds if q_bounds else [(-10, 10)]*q_lower_dim

        self.total_dim = p0_lower_dim + r_lower_dim + q_lower_dim

    def split_params(self, x):
        """
        Splits the flat array x into [p0_part, r_part, q_part].
        """
        p0_part = x[:self.p0_lower_dim]
        r_part = x[self.p0_lower_dim : self.p0_lower_dim+self.r_lower_dim]
        q_part = x[-self.q_lower_dim:]
        return p0_part, r_part, q_part


def build_matrix_from_lower_triangle(params, dim):
    """
    Rebuild a dim x dim lower-triangular matrix (or Cholesky factor) from a
    1D array 'params' of length dim*(dim+1)//2.
    """
    matrix = np.zeros((dim, dim), dtype=float)
    idx = 0
    for i in range(dim):
        for j in range(i+1):
            matrix[i, j] = params[idx]
            idx += 1
    return matrix
