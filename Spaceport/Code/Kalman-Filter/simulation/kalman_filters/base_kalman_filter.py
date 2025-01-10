import abc
import numpy as np

class BaseKalmanFilter(abc.ABC):
    """
    Abstract base class for any Kalman Filter. 
    This exposes the interface methods needed by all Kalman Filter variants.
    """
    @abc.abstractmethod
    def predict_state(self):
        pass

    @abc.abstractmethod
    def update_with_measurement(self, measurement):
        pass

    @abc.abstractmethod
    def iterate(self, dt, measurement, control):
        pass

    @abc.abstractmethod
    def get_state(self):
        """
        Returns the current estimated state as a numpy array.
        """
        pass

    @abc.abstractmethod
    def set_parameters(self, **kwargs):
        """
        Allows external objects (like an optimizer) to update filter parameters 
        (e.g., process_noise, measurement_noise, or covariance).
        """
        pass
