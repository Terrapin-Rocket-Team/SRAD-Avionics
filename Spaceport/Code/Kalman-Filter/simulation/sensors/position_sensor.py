import numpy as np
from .base_sensor import BaseSensor

class PositionSensor(BaseSensor):
    """
    Measures the position (x, y, z) from a 6D [x, y, z, vx, vy, vz] state.
    """

    def measure(self, state: np.ndarray) -> np.ndarray:
        true_position = state[:3]  # x, y, z
        if self.noise_generator is not None:
            noisy = self.noise_generator.apply_noise(true_position)
            return noisy.reshape(-1, 1)
        else:
            return true_position.reshape(-1, 1)
