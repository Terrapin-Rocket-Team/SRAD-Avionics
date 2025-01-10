import abc
import numpy as np

class BaseNoiseGenerator(abc.ABC):
    @abc.abstractmethod
    def apply_noise(self, data: np.ndarray) -> np.ndarray:
        """
        Given input data, returns the noisy version of that data.
        """
        pass
