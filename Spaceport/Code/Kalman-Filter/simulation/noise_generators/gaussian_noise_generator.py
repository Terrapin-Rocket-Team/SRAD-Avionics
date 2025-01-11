import numpy as np
from .base_noise_generator import BaseNoiseGenerator

class GaussianNoiseGenerator(BaseNoiseGenerator):
    def __init__(self, sigma: float):
        """
        :param sigma: standard deviation of the Gaussian noise
        """
        self.sigma = sigma

    def apply_noise(self, data: np.ndarray) -> np.ndarray:
        noise = self.sigma * np.random.randn(*data.shape)
        return data + noise
