import abc
import numpy as np
from typing import Optional

import sys, os      # for importing from parent directory (kinda hacky but wtv)
sys.path.insert(0, os.path.abspath('..'))

from simulation.noise_generators.base_noise_generator import BaseNoiseGenerator

class BaseSensor(abc.ABC):
    """
    Abstract sensor that observes some part of the rocket state.
    A sensor can have its own noise model.
    """
    def __init__(self, noise_generator: Optional[BaseNoiseGenerator] = None):
        self.noise_generator = noise_generator

    @abc.abstractmethod
    def measure(self, state: np.ndarray) -> np.ndarray:
        """
        Observes the rocket's true state and returns measurement 
        (potentially with noise).
        """
        pass
