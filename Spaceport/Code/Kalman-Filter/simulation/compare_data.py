import numpy as np
import matplotlib.pyplot as plt
import plot_managers.plot_manager as pm
import plot_managers.comparative_plot_manager as cpm

# Import your classes
from models.new_rocket import NewRocket
from data_generators.rocket_data_generator import RocketDataGenerator
from data_generators.real_flight_data_loader import RealFlightDataLoader
from data_generators.extended_rocket_data_generator import EnhancedRocketDataGenerator

from noise_generators.gaussian_noise_generator import GaussianNoiseGenerator
# from noise_generators.pink_noise_generator import PinkNoiseGenerator
# from noise_generators.drift_noise_generator import DriftNoiseGenerator

from sensors.sensor import Sensor
# from sensors.acceleration_sensor import AccelerationSensor

def main():
    # 1. Initializes rocket model and local variables

    # Separate arrays for each data type
    sim_positions = []
    sim_accelerations = []
    real_positions = []
    real_accelerations = []

    rocket = NewRocket(
        motorAccel=125, 
        burnTime=4.0, 
        dragCoef=0.5, 
        length=3.3528,
        diameter=.1524,
        mass_empty=39.4625,
        mass_full=56.2455,
        surface_roughness=50e-6,
    )     

    # 2. Generate simulated data
    generator = EnhancedRocketDataGenerator(
        rocket=rocket, 
        loop_frequency=50, 
        pre_launch_delay=10,
        launch_angle=20,
        wind_affector=(lambda t: np.array([0 * np.sin(t), 0*np.cos(t), 0]))
    )
    sim_data = generator.generate()

    # Extract simulated ground truth
    sim_time_data = sim_data["time"]
    r_x, r_y, r_z = sim_data["r_x"], sim_data["r_y"], sim_data["r_z"]
    v_x, v_y, v_z = sim_data["v_x"], sim_data["v_y"], sim_data["v_z"]
    a_x, a_y, a_z = sim_data["a_x"], sim_data["a_y"], sim_data["a_z"] + 9.81  # Add gravity

    # For simulated data create "measured" values by adding noise
    position_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=1.0)])
    acceleration_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=0.1)])

    for i in range(len(sim_time_data)):
        # Simulated noisy data
        sim_positions.append(position_sensor.measure(np.array([r_x[i], r_y[i], r_z[i]])))
        sim_accelerations.append(acceleration_sensor.measure(np.array([a_x[i], a_y[i], a_z[i]])))

    # 3. Gather real flight data
    loader = RealFlightDataLoader(
        file_path="flight_data/2024_SAC_Flight_Data.csv",
        rocket=rocket,
        rescale_factor=1,
        pre_launch_cut=0.95,
        wind_affector=(lambda t: np.array([2 * np.sin(t), 0]))
    )
    real_data = loader.generate()

    # Store measured real data
    real_time_data = real_data["time"]
    real_r_x, real_r_y, real_r_z = real_data["r_x"], real_data["r_y"], real_data["r_z"]
    real_a_x, real_a_y, real_a_z = real_data["a_x"], real_data["a_y"], real_data["a_z"]

    # make measured position and accelerations a list of 3D numpy arrays
    for i in range(len(real_time_data)):
         # Store real measured data separately
        if real_r_x is not None:
            real_positions.append(np.array([[real_r_x[i]], [real_r_y[i]], [real_r_z[i]]]).reshape(-1, 1))
        if real_a_x is not None:
            real_accelerations.append(np.array([[real_a_x[i]], [real_a_y[i]], [real_a_z[i]]]).reshape(-1, 1))

    print("Data processing complete.")

    # 4. Store information in dictionaries for plotting
    sim = {
    "time": sim_time_data,
    "r_x": r_x,
    "r_y": r_y,
    "r_z": r_z,
    "measured_r_x": [x[0] for x in sim_positions],
    "measured_r_y": [y[1] for y in sim_positions],
    "measured_r_z": [z[2] for z in sim_positions],
    
    # Check if estimated_positions are available
    "estimated_r_x": None,
    "estimated_r_y": None,
    "estimated_r_z": None,
    
    "v_x": v_x,
    "v_y": v_y,
    "v_z": v_z,
    
    # Check if estimated_velocities are available
    "estimated_v_x": None,
    "estimated_v_y": None,
    "estimated_v_z": None,
    
    "a_x": a_x,
    "a_y": a_y,
    "a_z": a_z,
    
    # Check if measured_accelerations are available
    "measured_a_x": [x[0] for x in sim_accelerations],
    "measured_a_y": [y[1] for y in sim_accelerations],
    "measured_a_z": [z[2] for z in sim_accelerations]
    }

    real = {
    "time": real_time_data,
    "r_x": real_r_x,
    "r_y": real_r_y,
    "r_z": real_r_z,
    "measured_r_x": [x[0] for x in real_positions],
    "measured_r_y": [y[1] for y in real_positions],
    "measured_r_z": [z[2] for z in real_positions],
    
    # Check if estimated_positions are available
    "estimated_r_x": None,
    "estimated_r_y": None,
    "estimated_r_z": None,
    
    "v_x": None,
    "v_y": None,
    "v_z": None,
    
    # Check if estimated_velocities are available
    "estimated_v_x": None,
    "estimated_v_y": None,
    "estimated_v_z": None,
    
    "a_x": real_a_x,
    "a_y": real_a_y,
    "a_z": real_a_z,
    
    # Check if measured_accelerations are available
    "measured_a_x": [x[0] for x in real_accelerations],
    "measured_a_y": [y[1] for y in real_accelerations],
    "measured_a_z": [z[2] for z in real_accelerations]
    }

    # 5. Compare plots to make sure that simulated data is a similar shape, etc. to real data
    
    sim_manager = pm.PlotManager(sim, run_kf=False)  # run_kf=False since estimated values are None
    real_manager = pm.PlotManager(real, run_kf=False)

    # Create the comparative plot manager
    comparative = cpm.ComparativePlotManager(
        plot_managers=[sim_manager, real_manager],
        labels=["Simulation", "Real Data"]
    )

    comparative.compare_z_positions()
    comparative.compare_xyz_positions()
    # comparative.compare_mach_numbers()

if __name__ == "__main__":
    main()
