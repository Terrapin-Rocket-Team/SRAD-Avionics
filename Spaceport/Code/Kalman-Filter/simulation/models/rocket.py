import numpy as np

class Rocket:
    """
    Simple rocket model holding physical parameters.
    """
    def __init__(self, motorAccel, burnTime, dragCoef, crossSectionalArea):
        self.motorAccel = motorAccel
        self.burnTime = burnTime
        self.dragCoef = dragCoef
        self.crossSectionalArea = crossSectionalArea

    @property
    def rocket_info(self):
        return (
            f"Rocket - Motor Accel: {self.motorAccel}, "
            f"Burn Time: {self.burnTime}, "
            f"Drag Coeff: {self.dragCoef}, "
            f"Cross Section Area: {self.crossSectionalArea}"
        )
