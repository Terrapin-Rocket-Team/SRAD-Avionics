# data_generators/rocket_data_generator.py

import numpy as np
from .base_data_generator import BaseDataGenerator
from typing import Optional, Union, Callable

class RocketDataGenerator(BaseDataGenerator):
    """
    Generates simulated rocket flight data (the 'ground truth').
    Supports a pre-launch delay, launch angle, and wind effects.
    """

    def __init__(self, rocket, loop_frequency: float = 50, pre_launch_delay: float = 0.0,
                 launch_angle: float = 0.0,  # in degrees
                 wind_affector: Optional[Union[np.ndarray, Callable[[float], np.ndarray]]] = None):
        """
        :param rocket: A rocket object describing the physical parameters.
        :param loop_frequency: Frequency at which data is generated (Hz).
        :param pre_launch_delay: How many seconds the rocket sits idle before launching.
        :param launch_angle: Launch angle relative to the vertical (degrees). Picks a random heading angle for the launch.
        :param wind_affector: Either a fixed 2D numpy array/list or a callable that takes time and returns a 2D numpy array
        """
        self.rocket = rocket
        self.loop_frequency = loop_frequency
        self.pre_launch_delay = pre_launch_delay
        self.launch_angle = np.deg2rad(launch_angle)  # Convert to radians
        self.wind_affector = wind_affector
        self.drag_coef = rocket.dragCoef
        self.cross_sectional_area = rocket.topCrossSectionalArea        # due to wind speed, the top dominates
        self.rho = 1.225  # Air density in kg/m^3

        if self.wind_affector is not None and self.rocket.mass is None:
            raise ValueError("Rocket mass must be defined in the Rocket object when using a wind affector.")

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
            a_z.append(-9.81)  # Gravity

        # 2. Generate flight data
        i = pre_launch_steps
        burn_time = self.rocket.burnTime
        motor_accel = self.rocket.motorAccel
        mass = self.rocket.mass

        # Initialize
        current_r_z = 0.1  # Starting on pad, slightly above ground
        current_v_z = 0.0
        current_r_x = 0.0
        current_r_y = 0.0
        current_v_x = 0.0
        current_v_y = 0.0

        while current_r_z > 0 or i == pre_launch_steps:
            t_val = i * dt
            time_list.append(t_val)

            # Determine thrust acceleration based on launch angle
            if t_val < burn_time + self.pre_launch_delay:

                # pick a heading angle for the rocket
                if self.launch_angle != 0:
                    heading_angle = np.random.uniform(0, 2*np.pi)   # angle from x-axis
                else:
                    heading_angle = 0

                # Thrust is active
                thrust_accel_x = motor_accel * np.sin(self.launch_angle) * np.cos(heading_angle)
                thrust_accel_y = motor_accel * np.sin(self.launch_angle) * np.sin(heading_angle)
                thrust_accel_z = motor_accel * np.cos(self.launch_angle) - 9.81  # Gravity
            else:
                # Thrust is inactive
                thrust_accel_x = 0.0
                thrust_accel_y = 0.0
                thrust_accel_z = -9.81  # Gravity

            # Apply drag based on current velocity
            velocity_mag = np.sqrt(current_v_x**2 + current_v_y**2 + current_v_z**2)
            if velocity_mag != 0:
                drag_accel_x = -0.5 * self.rho * self.drag_coef * self.cross_sectional_area * current_v_x * velocity_mag / mass
                drag_accel_y = -0.5 * self.rho * self.drag_coef * self.cross_sectional_area * current_v_y * velocity_mag / mass
                drag_accel_z = -0.5 * self.rho * self.drag_coef * self.cross_sectional_area * current_v_z * velocity_mag / mass
            else:
                drag_accel_x = 0.0
                drag_accel_y = 0.0
                drag_accel_z = 0.0

            # Initialize wind acceleration
            a_wind_x = 0.0
            a_wind_y = 0.0

            # Calculate wind-induced acceleration if wind_affector is provided
            if self.wind_affector is not None:
                if callable(self.wind_affector):
                    wind_vector = self.wind_affector(t_val)
                else:
                    wind_vector = self.wind_affector

                wind_x, wind_y = wind_vector

                # Calculate drag force due to wind
                F_wind_x = 0.5 * self.rho * self.drag_coef * self.cross_sectional_area * wind_x * np.abs(wind_x)
                F_wind_y = 0.5 * self.rho * self.drag_coef * self.cross_sectional_area * wind_y * np.abs(wind_y)

                # Calculate acceleration due to wind
                a_wind_x = F_wind_x / mass
                a_wind_y = F_wind_y / mass

            # Total acceleration
            total_a_x = thrust_accel_x + drag_accel_x + a_wind_x
            total_a_y = thrust_accel_y + drag_accel_y + a_wind_y
            total_a_z = thrust_accel_z + drag_accel_z

            # Update velocities
            current_v_x += total_a_x * dt
            current_v_y += total_a_y * dt
            current_v_z += total_a_z * dt

            # Update positions
            current_r_x += current_v_x * dt + 0.5 * total_a_x * dt**2
            current_r_y += current_v_y * dt + 0.5 * total_a_y * dt**2
            current_r_z += current_v_z * dt + 0.5 * total_a_z * dt**2

            # Store data
            r_x.append(current_r_x)
            r_y.append(current_r_y)
            r_z.append(current_r_z)
            v_x.append(current_v_x)
            v_y.append(current_v_y)
            v_z.append(current_v_z)
            a_x.append(total_a_x)
            a_y.append(total_a_y)
            a_z.append(total_a_z)

            i += 1
            if current_r_z < 0 and i > pre_launch_steps:
                break  # Rocket has landed

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
