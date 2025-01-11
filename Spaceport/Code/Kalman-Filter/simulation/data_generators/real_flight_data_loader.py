import pandas as pd
import numpy as np
from .base_data_generator import BaseDataGenerator

class RealFlightDataLoader(BaseDataGenerator):
    """
    Loads real flight data from a CSV, optionally applies augmentation 
    (rescaling, trimming pre-launch data).
    """

    def __init__(self, file_path: str, 
                 rescale_factor: float = 1.0,
                 pre_launch_cut: float = 0.0):
        """
        :param file_path: path to CSV data
        :param rescale_factor: factor to multiply the positions/velocities/accelerations by
        :param pre_launch_cut: percentage of pre-launch data from on the pad to cut out (0.0 to 1.0)
        """
        self.file_path = file_path
        self.rescale_factor = rescale_factor
        self.pre_launch_cut = pre_launch_cut

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

        time_s = time_s[start_index:] - self.pre_launch_cut
        r_x_raw = r_x_raw[start_index:]
        r_y_raw = r_y_raw[start_index:]
        r_z_raw = r_z_raw[start_index:]
        a_x_raw = a_x_raw[start_index:]
        a_y_raw = a_y_raw[start_index:]
        a_z_raw = a_z_raw[start_index:]

        # 2. Rescale if needed
        r_x_scaled = r_x_raw * self.rescale_factor
        r_y_scaled = r_y_raw * self.rescale_factor
        r_z_scaled = r_z_raw * self.rescale_factor
        a_x_scaled = a_x_raw * self.rescale_factor
        a_y_scaled = a_y_raw * self.rescale_factor
        a_z_scaled = a_z_raw * self.rescale_factor

        return {
            "time": time_s,
            "r_x": r_x_scaled,
            "r_y": r_y_scaled,
            "r_z": r_z_scaled,
            "a_x": a_x_scaled,
            "a_y": a_y_scaled,
            "a_z": a_z_scaled,
        }
