import numpy as np
from .base_sensor import BaseSensor

class AccelerationSensor(BaseSensor):
    """
    Measures the acceleration from some custom logic or a known state vector.
    If your state doesn't store acceleration directly, 
    you might compute it from external physics or store it in the state in some extended system.
    """

    def measure(self, acceleration: np.ndarray) -> np.ndarray:
        # Here, we assume acceleration is already part of the 'state', or we have it from rocket's model
        if self.noise_generator is not None:
            noisy = self.noise_generator.apply_noise(acceleration)
            return noisy.reshape(-1, 1)
        else:
            return acceleration.reshape(-1, 1)
