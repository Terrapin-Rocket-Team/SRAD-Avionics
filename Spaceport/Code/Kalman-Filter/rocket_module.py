#rocket.py
import numpy as np
class Rocket:
    def __init__(self,*args):

        if len(args) < 4:
            raise ValueError("Not all arguments were provided")
        
        self.a = args

        self.motorAccel = args[0]
        self.burnTime = args[1]
        self.dragCoef = args[2]
        self.crossSectionalArea = args[3]
        

    @property
    def rocket_info(self):
         return f"Rocket - Motor Accel: {self.motorAccel}, Burn Time: {self.burnTime}, Drag Coeff: {self.dragCoef}, Cross Section Area: {self.crossSectionalArea}"


# hardcoded values 
if __name__ == '__main__':
 rocket_instance = Rocket(9.8, 100, 0.5, 1.2)
 print(rocket_instance.rocket_info)
