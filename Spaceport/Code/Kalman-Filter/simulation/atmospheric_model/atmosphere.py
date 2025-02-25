from ambiance import Atmosphere
from typing import Optional, Union, Callable

class AtmosphereModel:
    """Models atmospheric conditions using COESA 1976 standard."""

    def __init__(self):
        self.R_earth = 6371000  # Mean radius of the Earth (m)
        self.g0 = 9.80665       # Standard gravity at sea level (m/s²)
        
    def get_conditions(self, 
                      altitude: float, 
                      velocity: Optional[float] = None,
                      characteristic_length: Optional[float] = None) -> dict:
        """
        :param altitude: Current altitude of Rocket (m)
        :param velocity: Magnitude of current velocity (m/s) (Optional).
        :param characteristic_length: for rockets typically diameter (Optional).
        """
        
        atmosphere = Atmosphere(altitude)
        gravity = self.g0 * (self.R_earth / (self.R_earth + altitude))**2
        
        reynolds = None
        if velocity is not None and characteristic_length is not None:
            reynolds = (atmosphere.density[0] * velocity * characteristic_length / 
                       atmosphere.dynamic_viscosity[0])
        
        return {
            "pressure": atmosphere.pressure[0],
            "temperature": atmosphere.temperature[0],
            "density": atmosphere.density[0],
            "speed_of_sound": atmosphere.speed_of_sound[0],
            "gravity": gravity,
            "reynolds_number": reynolds
        }