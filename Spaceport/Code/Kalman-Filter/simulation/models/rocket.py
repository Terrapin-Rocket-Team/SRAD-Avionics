# models/rocket.py

import numpy as np

class Rocket:
    """
    Simple rocket model holding physical parameters.
    """
    def __init__(self, motorAccel, burnTime, dragCoef, topCrossSectionalArea, sideCrossSectionalArea, mass):
        """
        :param motorAccel: Motor acceleration (m/s^2)
        :param burnTime: Duration of motor burn (s)
        :param dragCoef: Drag coefficient
        :param topCrossSectionalArea: Cross-sectional area exposed to z-direction wind (m^2)
        :param sideCrossSectionalArea: Cross-sectional area exposed to lateral face wind (m^2)
        :param mass: Mass of the rocket (kg)
        """
        self.motorAccel = motorAccel
        self.burnTime = burnTime
        self.dragCoef = dragCoef
        self.topCrossSectionalArea = topCrossSectionalArea
        self.sideCrossSectionalArea = sideCrossSectionalArea
        self.mass = mass

    @property
    def rocket_info(self):
        return (
            f"Rocket - Motor Accel: {self.motorAccel} m/s², "
            f"Burn Time: {self.burnTime} s, "
            f"Drag Coeff: {self.dragCoef}, "
            f"Cross Section Area: {self.topCrossSectionalArea} m², "
            f"Mass: {self.mass} kg"
        )
