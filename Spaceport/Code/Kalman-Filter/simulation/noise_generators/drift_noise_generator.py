import numpy as np
from .base_noise_generator import BaseNoiseGenerator

class DriftNoiseGenerator(BaseNoiseGenerator):
    """
    Adds a drift (slowly changing bias) to the measurements.
    """
    def __init__(self, drift_rate: float = 0.01):
        self.drift_rate = drift_rate
        self.current_drift = 0.0

    def apply_noise(self, data: np.ndarray) -> np.ndarray:
        # Over each call, we increment the drift
        drifted_data = []
        for val in data:
            self.current_drift += self.drift_rate
            drifted_data.append(val + self.current_drift)
        return np.array(drifted_data)
