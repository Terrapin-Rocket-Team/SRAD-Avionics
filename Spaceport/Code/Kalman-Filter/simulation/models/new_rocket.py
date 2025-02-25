import numpy as np

class NewRocket:
    """
    Simple rocket model holding physical parameters.
    """
    def __init__(self, motorAccel, burnTime, dragCoef, length, diameter, mass_empty, mass_full, surface_roughness = None, CoG = 106, CoP = 123):
        """
        :param motorAccel: Motor acceleration (m/s^2)
        :param burnTime: Duration of motor burn (s)
        :param dragCoef: Drag coefficient
        :param length: Total rocket length (m)
        :param diameter: Rocket body diameter (m)
        :param mass_empty: Mass without fuel (kg)
        :param mass_full: Mass of a full rocket (kg)
        :param surface_roughness: Roughness of surface (m)
        :param CoG: Center of gravity (m)
        :param CoP: Center of pressure (m)
        """
        self.motorAccel = motorAccel
        self.burnTime = burnTime
        self.dragCoef = dragCoef # Still added for unmigrated data 
        self.length = length
        self.diameter = diameter
        self.mass_empty = mass_empty
        self.mass = mass_full
        self.surface_roughness = surface_roughness
        self.CoG = CoG
        self.CoP = CoP
        # Calculates the mass of fuel
        self.mass_fuel = mass_full - mass_empty
        # Calculate cross-sectional areas
        self.topCrossSectionalArea = np.pi * (diameter/2)**2
        self.sideCrossSectionalArea = length * diameter

    @property
    def rocket_info(self):
        return (
            f"Rocket - Motor Accel: {self.motorAccel} m/s², "
            f"Burn Time: {self.burnTime} s, "
            f"Drag Coeff: {self.dragCoef}, "
            f"Cross Section Area: {self.topCrossSectionalArea} m², "
            f"Mass: {self.mass} kg"
        )

    def get_current_mass(self, time: float) -> float:
        """Calculate current mass based on fuel consumption."""
        if time < self.burnTime:
            remaining_fuel_ratio = (self.burnTime - time) / self.burnTime
            return self.mass_empty + self.mass_fuel * remaining_fuel_ratio
        return self.mass_empty