import matplotlib.pyplot as plt
import numpy as np
from atmospheric_model.atmosphere import AtmosphereModel

class PlotManager:
    """
    A utility class for managing and generating plots for simulation data.
    """

    def __init__(self, data_dict, run_kf=True):
        """
        :param data_dict: A dictionary containing time, position, velocity, and related data.
                          Keys should match the variable names used in the simulation.
        :param run_kf: A flag indicating if Kalman Filter outputs should be included in plots.
        :param atmosphere: atmospheric model
        """
        self.data = data_dict
        self.run_kf = run_kf
        self.plots = {}
        self.atmosphere = AtmosphereModel()

    def add_plot(self, plot_id, plot_function):
        """
        Add a custom plot to the manager.

        :param plot_id: A unique identifier for the plot.
        :param plot_function: A function that generates the plot.
        """
        self.plots[plot_id] = plot_function

    def remove_plot(self, plot_id):
        """
        Remove a plot from the manager.

        :param plot_id: The identifier of the plot to remove.
        """
        if plot_id in self.plots:
            del self.plots[plot_id]

    def show_plot(self, plot_id):
        """
        Show a specific plot by its ID.

        :param plot_id: The identifier of the plot to show.
        """
        if plot_id in self.plots:
            self.plots[plot_id]()
        else:
            print(f"Plot ID '{plot_id}' not found.")

    def show_all(self):
        """Generate and show all added plots."""
        for plot_id, plot_func in self.plots.items():
            print(f"Generating plot: {plot_id}")
            plot_func()

    def _get_data(self, key):
        """
        Safely retrieve data from the dictionary, handling missing keys.
        """
        return self.data.get(key, None)

# Example Plot Functions
def plot_z_position(manager):
    """Plot Z position vs Time."""
    time = manager._get_data("time")
    actual_z = manager._get_data("r_z")
    measured_z = manager._get_data("measured_r_z")
    estimated_z = manager._get_data("estimated_r_z")


    plt.figure()
    if actual_z is not None:
        plt.plot(time, actual_z, "r-", label="Actual Z Position")
    if measured_z is not None:
        plt.plot(time, measured_z, "g.", label="Measured Z Position")
    if manager.run_kf and estimated_z is not None:
        plt.plot(time[:-1], estimated_z, "b-", label="Estimated Z Position")        # estimated values have one less element
    plt.xlabel("Time (s)")
    plt.ylabel("Z Position (m)")
    plt.title("Z Position vs Time")
    plt.legend()
    plt.grid(True)
    plt.show()

def plot_xyz_position(manager):
    """Plot X, Y, Z positions vs Time (subplots)."""
    time = manager._get_data("time")
    actual_x = manager._get_data("r_x")
    actual_y = manager._get_data("r_y")
    actual_z = manager._get_data("r_z")
    measured_x = manager._get_data("measured_r_x")
    measured_y = manager._get_data("measured_r_y")
    measured_z = manager._get_data("measured_r_z")
    estimated_x = manager._get_data("estimated_r_x")
    estimated_y = manager._get_data("estimated_r_y")
    estimated_z = manager._get_data("estimated_r_z")

    plt.figure()

    # X Position
    plt.subplot(3, 1, 1)
    if actual_x is not None:
        plt.plot(time, actual_x, "r-", label="Actual X")
    if measured_x is not None:
        plt.plot(time, measured_x, "g.", label="Measured X")
    if manager.run_kf and estimated_x is not None:
        plt.plot(time[:-1], estimated_x, "b-", label="Estimated X")
    plt.xlabel("Time (s)")
    plt.ylabel("X Position (m)")
    plt.legend()
    plt.grid(True)

    # Y Position
    plt.subplot(3, 1, 2)
    if actual_y is not None:
        plt.plot(time, actual_y, "r-", label="Actual Y")
    if measured_y is not None:
        plt.plot(time, measured_y, "g.", label="Measured Y")
    if manager.run_kf and estimated_y is not None:
        plt.plot(time[:-1], estimated_y, "b-", label="Estimated Y")
    plt.xlabel("Time (s)")
    plt.ylabel("Y Position (m)")
    plt.legend()
    plt.grid(True)

    # Z Position
    plt.subplot(3, 1, 3)
    if actual_z is not None:
        plt.plot(time, actual_z, "r-", label="Actual Z")
    if measured_z is not None:
        plt.plot(time, measured_z, "g.", label="Measured Z")
    if manager.run_kf and estimated_z is not None:
        plt.plot(time[:-1], estimated_z, "b-", label="Estimated Z")
    plt.xlabel("Time (s)")
    plt.ylabel("Z Position (m)")
    plt.legend()
    plt.grid(True)

    plt.tight_layout()
    plt.show()

def plot_z_velocity(manager):
    """Plot Z velocity vs Time."""
    time = manager._get_data("time")
    actual_z_vel = manager._get_data("v_z")
    estimated_z_vel = manager._get_data("estimated_v_z")

    plt.figure()
    if actual_z_vel is not None:
        plt.plot(time, actual_z_vel, "r-", label="Actual Z Velocity")
    if manager.run_kf and estimated_z_vel is not None:
        plt.plot(time[:-1], estimated_z_vel, "b-", label="Estimated Z Velocity")
    plt.xlabel("Time (s)")
    plt.ylabel("Z Velocity (m/s)")
    plt.title("Z Velocity vs Time")
    plt.legend()
    plt.grid(True)
    plt.show()

def plot_mach_number(manager):
    """Plot Mach number vs Time based on velocity magnitude and atmospheric model."""
    time = manager._get_data("time")
    v_x = manager._get_data("v_x")
    v_y = manager._get_data("v_y")
    v_z = manager._get_data("v_z")
    r_z = manager._get_data("r_z") 

    if v_x is not None and v_y is not None and v_z is not None and r_z is not None:
        # Calculate velocity magnitude
        velocity_magnitude = np.sqrt(v_x**2 + v_y**2 + v_z**2)

        # Get speed of sound at each altitude
        speed_of_sound = np.array([manager.atmosphere.get_conditions(alt)["speed_of_sound"] for alt in r_z])

        # Ensure speed_of_sound has the correct shape
        if speed_of_sound.shape != velocity_magnitude.shape:
            print("Warning: Speed of sound shape does not match velocity magnitude. Check altitude inputs.")

        # Calculate Mach number
        mach_number = velocity_magnitude / speed_of_sound

        plt.figure()
        plt.plot(time, mach_number, "b-", label="Mach Number")
        plt.xlabel("Time (s)")
        plt.ylabel("Mach Number")
        plt.title("Mach Number vs Time")
        plt.legend()
        plt.grid(True)
        plt.show()
    else:
        print("Error: Missing velocity or altitude data.")