import abc
import numpy as np

class BaseDataGenerator(abc.ABC):
    """
    Abstract base class for all data generators.
    """
    @abc.abstractmethod
    def generate(self) -> dict:
        """
        Should return a dictionary (or DataFrame) containing 
        'time' and any relevant state variables (e.g., position, velocity, acceleration).
        """
        pass
