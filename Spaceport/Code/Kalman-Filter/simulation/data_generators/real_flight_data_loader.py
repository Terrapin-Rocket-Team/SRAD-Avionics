import pandas as pd
import numpy as np
from .base_data_generator import BaseDataGenerator
from typing import Optional, Union, Callable
from models.rocket import Rocket

class RealFlightDataLoader(BaseDataGenerator):
    """
    Loads real flight data from a CSV, optionally applies augmentation 
    (rescaling, trimming pre-launch data).
    """

    def __init__(self, file_path: str, 
                 rocket: Rocket,
                 rescale_factor: float = 1.0,
                 pre_launch_cut: float = 0.0,
                 wind_affector: Optional[Union[np.ndarray, Callable[[float], np.ndarray]]] = None):
        """
        :param file_path: path to CSV data
        :param rescale_factor: factor to multiply the positions/velocities/accelerations by
        :param pre_launch_cut: percentage of pre-launch data from on the pad to cut out (0.0 to 1.0)
        :param wind_affector: optional wind affector, either a 2D numpy array or a function of time that returns a 2D numpy array.
        Represents the wind vector that affects the rocket's flight (m/s).
        """
        self.file_path = file_path
        self.rescale_factor = rescale_factor
        self.pre_launch_cut = pre_launch_cut
        self.wind_affector = wind_affector
        self.rocket = rocket
        self.rho = 1.225  # air density (kg/m^3)

    def generate(self) -> dict:
        df = pd.read_csv(self.file_path)

        # Assume your CSV columns: Time (ms), PosX (m), PosY (m), PosZ (m), etc.
        # Adjust to your actual column names.
        time_s = df["t"].to_numpy(dtype=float)
        r_x_raw = df["r_x"].to_numpy(dtype=float)
        r_y_raw = df["r_y"].to_numpy(dtype=float)
        r_z_raw = df["r_z"].to_numpy(dtype=float)
        a_x_raw = df["a_x"].to_numpy(dtype=float)
        a_y_raw = df["a_y"].to_numpy(dtype=float)
        a_z_raw = df["a_z"].to_numpy(dtype=float)

        # 1. Trim out pre-launch time

        # figure out the index of launch based on when the a_z_raw is above a threshold and r_z_raw is above a threshold
        acceleration_threshold = 15.0  # Adjust based on your system's expected launch acceleration (e.g., 1 m/s^2)

        # Find the launch index
        launch_index = np.argmax((abs(a_z_raw) > acceleration_threshold))

        # find out how many indices to cut out based on pre_launch_cut
        start_index = int(self.pre_launch_cut * launch_index)

        time_s = time_s[start_index:]
        r_x_raw = r_x_raw[start_index:]
        r_y_raw = r_y_raw[start_index:]
        r_z_raw = r_z_raw[start_index:]
        a_x_raw = a_x_raw[start_index:]
        a_y_raw = a_y_raw[start_index:]
        a_z_raw = a_z_raw[start_index:]

        time_s = time_s - time_s[0]

        # 2. Rescale if needed
        r_x_scaled = r_x_raw * self.rescale_factor
        r_y_scaled = r_y_raw * self.rescale_factor
        r_z_scaled = r_z_raw * self.rescale_factor
        a_x_scaled = a_x_raw * self.rescale_factor
        a_y_scaled = a_y_raw * self.rescale_factor
        a_z_scaled = a_z_raw * self.rescale_factor

        # 3. Apply wind affector if needed
        if self.wind_affector is not None:
            dt = np.diff(time_s, prepend=0)  # Time step, with prepend to align indices
            for i in range(launch_index - start_index, len(time_s)):
                current_time = time_s[i]
                # Get wind vector
                if callable(self.wind_affector):
                    wind_vector = self.wind_affector(current_time)
                else:
                    wind_vector = self.wind_affector

                wind_x, wind_y = wind_vector

                # Calculate drag force
                F_drag_x = 0.5 * self.rho * self.rocket.dragCoef * self.rocket.sideCrossSectionalArea * wind_x * np.abs(wind_x)
                F_drag_y = 0.5 * self.rho * self.rocket.dragCoef * self.rocket.sideCrossSectionalArea * wind_y * np.abs(wind_y)

                # Calculate acceleration due to wind
                a_wind_x = F_drag_x / self.rocket.mass
                a_wind_y = F_drag_y / self.rocket.mass

                # Update accelerations
                a_x_scaled[i] += a_wind_x
                a_y_scaled[i] += a_wind_y

                # Update velocities by integrating acceleration
                if i == 0:
                    v_x = a_x_scaled[i] * dt[i]
                    v_y = a_y_scaled[i] * dt[i]
                else:
                    v_x = a_x_scaled[i] * dt[i] + (a_x_scaled[i-1] * dt[i-1])
                    v_y = a_y_scaled[i] * dt[i] + (a_y_scaled[i-1] * dt[i-1])

                # Update positions by integrating velocity
                r_x_scaled[i] += v_x * dt[i] + 0.5 * a_x_scaled[i] * dt[i]**2
                r_y_scaled[i] += v_y * dt[i] + 0.5 * a_y_scaled[i] * dt[i]**2

        return {
            "time": time_s,
            "r_x": r_x_scaled,
            "r_y": r_y_scaled,
            "r_z": r_z_scaled,
            "a_x": a_x_scaled,
            "a_y": a_y_scaled,
            "a_z": a_z_scaled,
        }
