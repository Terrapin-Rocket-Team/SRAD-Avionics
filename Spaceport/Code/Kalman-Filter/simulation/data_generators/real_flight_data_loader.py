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
        :param pre_launch_cut: how many seconds to cut from the start as pre-launch
        """
        self.file_path = file_path
        self.rescale_factor = rescale_factor
        self.pre_launch_cut = pre_launch_cut

    def generate(self) -> dict:
        df = pd.read_csv(self.file_path)

        # Assume your CSV columns: Time (ms), PosX (m), PosY (m), PosZ (m), etc.
        # Adjust to your actual column names.
        time_raw = df["Time (ms)"].to_numpy(dtype=float)
        r_x_raw = df["PosX (m)"].to_numpy(dtype=float)
        r_y_raw = df["PosY (m)"].to_numpy(dtype=float)
        r_z_raw = df["PosZ (m)"].to_numpy(dtype=float)
        v_x_raw = df["VeloX (m/s)"].to_numpy(dtype=float)
        v_y_raw = df["VeloY (m/s)"].to_numpy(dtype=float)
        v_z_raw = df["VeloZ (m/s)"].to_numpy(dtype=float)
        a_x_raw = df["AccelX (m/s^2)"].to_numpy(dtype=float)
        a_y_raw = df["AccelY (m/s^2)"].to_numpy(dtype=float)
        a_z_raw = df["AccelZ (m/s^2)"].to_numpy(dtype=float)

        # Convert time from ms to s, if needed
        time_s = time_raw / 1000.0

        # 1. Trim out pre-launch time
        # find the first index where time_s >= pre_launch_cut
        start_index = 0
        for i, t in enumerate(time_s):
            if t >= self.pre_launch_cut:
                start_index = i
                break

        time_s = time_s[start_index:] - self.pre_launch_cut
        r_x_raw = r_x_raw[start_index:]
        r_y_raw = r_y_raw[start_index:]
        r_z_raw = r_z_raw[start_index:]
        v_x_raw = v_x_raw[start_index:]
        v_y_raw = v_y_raw[start_index:]
        v_z_raw = v_z_raw[start_index:]
        a_x_raw = a_x_raw[start_index:]
        a_y_raw = a_y_raw[start_index:]
        a_z_raw = a_z_raw[start_index:]

        # 2. Rescale if needed
        r_x_scaled = r_x_raw * self.rescale_factor
        r_y_scaled = r_y_raw * self.rescale_factor
        r_z_scaled = r_z_raw * self.rescale_factor
        v_x_scaled = v_x_raw * self.rescale_factor
        v_y_scaled = v_y_raw * self.rescale_factor
        v_z_scaled = v_z_raw * self.rescale_factor
        a_x_scaled = a_x_raw * self.rescale_factor
        a_y_scaled = a_y_raw * self.rescale_factor
        a_z_scaled = a_z_raw * self.rescale_factor

        return {
            "time": time_s,
            "r_x": r_x_scaled,
            "r_y": r_y_scaled,
            "r_z": r_z_scaled,
            "v_x": v_x_scaled,
            "v_y": v_y_scaled,
            "v_z": v_z_scaled,
            "a_x": a_x_scaled,
            "a_y": a_y_scaled,
            "a_z": a_z_scaled,
        }
