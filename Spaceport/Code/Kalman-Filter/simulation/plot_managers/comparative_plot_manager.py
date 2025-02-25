import matplotlib.pyplot as plt
import math
import numpy as np
from atmospheric_model.atmosphere import AtmosphereModel

class ComparativePlotManager:
    """
    A class for comparing plots from multiple PlotManager instances side by side.
    """
    
    def __init__(self, plot_managers, labels=None):
        """
        Initialize the comparative plot manager.
        
        :param plot_managers: List of PlotManager instances to compare
        :param labels: Optional list of labels for each plot manager
        """
        self.plot_managers = plot_managers
        self.labels = labels or [f"Run {i+1}" for i in range(len(plot_managers))]
        if len(self.labels) != len(plot_managers):
            raise ValueError("Number of labels must match number of plot managers")
            
    def _calculate_subplot_layout(self, n):
        """
        Calculate the optimal layout for n subplots.
        
        :param n: Number of plots
        :return: Tuple of (rows, cols)
        """
        cols = math.ceil(math.sqrt(n))
        rows = math.ceil(n / cols)
        return rows, cols
        
    def compare_z_positions(self):
        """Compare Z position plots from all plot managers."""
        rows, cols = self._calculate_subplot_layout(len(self.plot_managers))
        fig = plt.figure(figsize=(6*cols, 4*rows))
        
        for idx, (manager, label) in enumerate(zip(self.plot_managers, self.labels)):
            plt.subplot(rows, cols, idx + 1)
            
            time = manager._get_data("time")
            actual_z = manager._get_data("r_z")
            measured_z = manager._get_data("measured_r_z")
            estimated_z = manager._get_data("estimated_r_z")
            
            if actual_z is not None:
                plt.plot(time, actual_z, "r-", label="Actual")
            if measured_z is not None:
                plt.plot(time, measured_z, "g.", label="Measured")
            if manager.run_kf and estimated_z is not None:
                plt.plot(time[:-1], estimated_z, "b-", label="Estimated")
                
            plt.xlabel("Time (s)")
            plt.ylabel("Z Position (m)")
            plt.title(f"Z Position vs Time - {label}")
            plt.legend()
            plt.grid(True)
            
        plt.tight_layout()
        plt.show()
        
    def compare_xyz_positions(self):
        """Compare XYZ position plots from all plot managers."""
        total_rows = len(self.plot_managers) * 3
        fig = plt.figure(figsize=(12, 4*total_rows))
        
        for manager_idx, (manager, label) in enumerate(zip(self.plot_managers, self.labels)):
            base_idx = manager_idx * 3
            time = manager._get_data("time")
            
            # X Position
            plt.subplot(total_rows, 1, base_idx + 1)
            if manager._get_data("r_x") is not None:
                plt.plot(time, manager._get_data("r_x"), "r-", label="Actual")
            if manager._get_data("measured_r_x") is not None:
                plt.plot(time, manager._get_data("measured_r_x"), "g.", label="Measured")
            if manager.run_kf and manager._get_data("estimated_r_x") is not None:
                plt.plot(time[:-1], manager._get_data("estimated_r_x"), "b-", label="Estimated")
            plt.ylabel("X Position (m)")
            plt.title(f"X Position vs Time - {label}")
            plt.legend()
            plt.grid(True)
            
            # Y Position
            plt.subplot(total_rows, 1, base_idx + 2)
            if manager._get_data("r_y") is not None:
                plt.plot(time, manager._get_data("r_y"), "r-", label="Actual")
            if manager._get_data("measured_r_y") is not None:
                plt.plot(time, manager._get_data("measured_r_y"), "g.", label="Measured")
            if manager.run_kf and manager._get_data("estimated_r_y") is not None:
                plt.plot(time[:-1], manager._get_data("estimated_r_y"), "b-", label="Estimated")
            plt.ylabel("Y Position (m)")
            plt.legend()
            plt.grid(True)
            
            # Z Position
            plt.subplot(total_rows, 1, base_idx + 3)
            if manager._get_data("r_z") is not None:
                plt.plot(time, manager._get_data("r_z"), "r-", label="Actual")
            if manager._get_data("measured_r_z") is not None:
                plt.plot(time, manager._get_data("measured_r_z"), "g.", label="Measured")
            if manager.run_kf and manager._get_data("estimated_r_z") is not None:
                plt.plot(time[:-1], manager._get_data("estimated_r_z"), "b-", label="Estimated")
            plt.xlabel("Time (s)")
            plt.ylabel("Z Position (m)")
            plt.legend()
            plt.grid(True)
            
        plt.tight_layout()
        plt.show()
        
    def compare_z_velocities(self):
        """Compare Z velocity plots from all plot managers."""
        rows, cols = self._calculate_subplot_layout(len(self.plot_managers))
        fig = plt.figure(figsize=(6*cols, 4*rows))
        
        for idx, (manager, label) in enumerate(zip(self.plot_managers, self.labels)):
            plt.subplot(rows, cols, idx + 1)
            
            time = manager._get_data("time")
            actual_z_vel = manager._get_data("v_z")
            estimated_z_vel = manager._get_data("estimated_v_z")
            
            if actual_z_vel is not None:
                plt.plot(time, actual_z_vel, "r-", label="Actual")
            if manager.run_kf and estimated_z_vel is not None:
                plt.plot(time[:-1], estimated_z_vel, "b-", label="Estimated")
                
            plt.xlabel("Time (s)")
            plt.ylabel("Z Velocity (m/s)")
            plt.title(f"Z Velocity vs Time - {label}")
            plt.legend()
            plt.grid(True)
            
        plt.tight_layout()
        plt.show()
