import numpy as np
from .base_data_generator import BaseDataGenerator

class RocketDataGenerator(BaseDataGenerator):
    """
    Generates simulated rocket flight data (the 'ground truth').
    Supports a pre-launch delay, where the rocket sits on the pad for some time.
    """

    def __init__(self, rocket, loop_frequency: float = 50, pre_launch_delay: float = 0.0):
        """
        :param rocket: A rocket object describing the physical parameters.
        :param loop_frequency: frequency at which data is generated.
        :param pre_launch_delay: how many seconds the rocket sits idle before launching.
        """
        self.rocket = rocket
        self.loop_frequency = loop_frequency
        self.pre_launch_delay = pre_launch_delay

    def generate(self) -> dict:
        dt = 1.0 / self.loop_frequency
        time_list = []
        r_x = []
        r_y = []
        r_z = []
        v_x = []
        v_y = []
        v_z = []
        a_x = []
        a_y = []
        a_z = []

        # 1. Generate pre-launch data (rocket not moving)
        pre_launch_steps = int(self.pre_launch_delay / dt)
        for i in range(pre_launch_steps):
            t_val = i * dt
            time_list.append(t_val)
            r_x.append(0.0)
            r_y.append(0.0)
            r_z.append(0.0)
            v_x.append(0.0)
            v_y.append(0.0)
            v_z.append(0.0)
            a_x.append(0.0)
            a_y.append(0.0)
            a_z.append(-9.81)  # still gravity, but rocket is held in place

        # 2. Generate flight data
        i = pre_launch_steps
        density = 1.225
        burn_time = self.rocket.burnTime
        motor_accel = self.rocket.motorAccel
        drag_coef = self.rocket.dragCoef
        cross_area = self.rocket.crossSectionalArea
        
        # Initialize
        current_r_z = 0.1  # starting on pad, slightly above ground
        current_v_z = 0.0
        # For x, y keep it 0 if you want.
        current_r_x = 0.0
        current_r_y = 0.0
        current_v_x = 0.0
        current_v_y = 0.0

        while current_r_z > 0 or i == pre_launch_steps:
            t_val = i * dt
            time_list.append(t_val)

            # Determine acceleration
            if current_v_z < 0:
                aoa = 1
            else:
                aoa = -1

            if t_val < burn_time + self.pre_launch_delay:
                accel_z = (motor_accel + aoa * 0.5 * density * drag_coef * cross_area * current_v_z) - 9.81
            else:
                accel_z = (aoa * 0.5 * density * drag_coef * cross_area * current_v_z) - 9.81

            # Update velocities
            current_v_z += accel_z * dt

            # Update positions
            current_r_z += current_v_z * dt

            # Store
            r_x.append(current_r_x)
            r_y.append(current_r_y)
            r_z.append(current_r_z)
            v_x.append(current_v_x)
            v_y.append(current_v_y)
            v_z.append(current_v_z)
            a_x.append(0)
            a_y.append(0)
            a_z.append(accel_z)

            i += 1
            if current_r_z < 0:
                break  # rocket has come back down

        return {
            "time": np.array(time_list),
            "r_x": np.array(r_x),
            "r_y": np.array(r_y),
            "r_z": np.array(r_z),
            "v_x": np.array(v_x),
            "v_y": np.array(v_y),
            "v_z": np.array(v_z),
            "a_x": np.array(a_x),
            "a_y": np.array(a_y),
            "a_z": np.array(a_z),
        }
