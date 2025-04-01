import abc
import numpy as np
from typing import Optional, List

import sys, os      # for importing from parent directory (kinda hacky but wtv)
sys.path.insert(0, os.path.abspath('..'))

from noise_generators.base_noise_generator import BaseNoiseGenerator

class Sensor:
    """
    Sensor class that represents a sensor that observes some part of the rocket state.
    A sensor can have its own noise model.
    """
    def __init__(self, noise_generators: Optional[List[BaseNoiseGenerator]] = []):
        self.noise_generator = noise_generators

    def measure(self, state: np.ndarray) -> np.ndarray:
        """
        Observes the state variables provided to it and returns measurement.

        Each noise model in the list of noise_generators will generate noise off the 
        ground truth data, and the final measurement will be the sum of all the noise.
        """
        noise = np.zeros_like(state)
        for noise_generator in self.noise_generator:
            noise += noise_generator.apply_noise(state) - state

        return (state + noise).reshape(-1, 1)

    def progressive_measure(self, state: np.ndarray, time_step: int) -> np.ndarray:
        """
        Observes the state variables provided to it and returns measurement.

        Each noise model in the list of noise_generators will generate noise, building up
        on top of the previous noise. This is useful for simulating noise that changes over time 
        or are dependent on previous noise.
        """
        noisy = state
        for noise_generator in self.noise_generator:
            noisy = noise_generator.apply_noise(noisy, time_step)
        return noisy.reshape(-1, 1)
