# main.py

import numpy as np
import matplotlib.pyplot as plt
import plot_managers.plot_manager as pm
import os

# Import your classes
from models.rocket import Rocket
from data_generators.rocket_data_generator import RocketDataGenerator
from data_generators.real_flight_data_loader import RealFlightDataLoader

from kalman_filters.avionics_kalman_filter import AvionicsKalmanFilter

from noise_generators.gaussian_noise_generator import GaussianNoiseGenerator
# from noise_generators.pink_noise_generator import PinkNoiseGenerator
# from noise_generators.drift_noise_generator import DriftNoiseGenerator

from sensors.sensor import Sensor
# from sensors.acceleration_sensor import AccelerationSensor

from optimizers.bayesian_optimizer import BayesianOptimizer
from optimizers.objective import objective_function_factory

import skopt
from skopt.space import Real

def main():
    # 1. Define real dataset files
    real_dataset_files = [
        "flight_data/2024_SAC_Flight_Data.csv",
        # Add more file paths as needed
    ]
    
    # 2. Define generated dataset configurations (parameter ranges)
    burn_time_range = (2.0, 5.0)                    # seconds
    mass_range = (30.0, 60.0)                       # kg
    launch_angle_range = (0.0, 10.0)                # degrees
    motor_accel_range = (30, 120)                   # m/s^2
    drag_coef_range = (0.8, 1.2)                    # unitless
    top_cross_sectional_area_range = (0.0, 0.15)    # m^2
    side_cross_sectional_area_range = (0.3, 0.8)    # m^2
    pos_noise_sigma_range = (0.0, 1.5)              # MAX-M10S has max 1.5 m error
    accel_noise_sigma_range = (0.0, 0.15)           # BMI088 has 230 µg/√Hz noise density
    flight_rescale_range=(0.3, 4)                   # Rescale factor for flight data
    pre_launch_cut_range=(0.8, 0.99)                # Cut out 80-99% of pre-launch data
    wind_affector_range=(0, 2.5)                    # Up to 2.5 m/s (5.6 mph) wind in each direction

    # 3. Define the search space for Bayesian Optimization
    # P: 6x6 lower triangular (21 parameters)
    # R: 3x3 lower triangular (6 parameters)
    # Q: 6x6 lower triangular (21 parameters)
    # Total: 48 parameters

    search_space = []
    # Define ranges for P (initial covariance)
    for i in range(6):
        for j in range(i+1):
            search_space.append(Real(low=1e-3, high=1e3, prior='uniform', name=f"P_{i}_{j}"))
    
    # Define ranges for R (measurement noise)
    for i in range(3):
        for j in range(i+1):
            search_space.append(Real(low=1e-3, high=1e3, prior='uniform', name=f"R_{i}_{j}"))
    
    # Define ranges for Q (process noise)
    for i in range(6):
        for j in range(i+1):
            search_space.append(Real(low=1e-3, high=1e3, prior='uniform', name=f"Q_{i}_{j}"))

    # 4. Create an instance of the Bayesian Optimizer
    objective_fn = objective_function_factory(
        real_dataset_files=real_dataset_files,
        n_real=5,
        n_generated=5,
        burn_time_range=burn_time_range,
        mass_range=mass_range,
        launch_angle_range=launch_angle_range,
        motor_accel_range=motor_accel_range,
        drag_coef_range=drag_coef_range,
        top_cross_sectional_area_range=top_cross_sectional_area_range,
        side_cross_sectional_area_range=side_cross_sectional_area_range,
        pos_noise_sigma_range=pos_noise_sigma_range,
        accel_noise_sigma_range=accel_noise_sigma_range,
        flight_rescale_range=flight_rescale_range,
        pre_launch_cut_range=pre_launch_cut_range,
        wind_affector_range=wind_affector_range,
        optimizer_seed=42
    )

    # starting point
    initial_initial_covariance = 1 * np.eye(6)
    initial_measurement_noise = 0.5 * np.eye(3)
    initial_process_noise = 0.1 * np.eye(6)

    # unravel the initial parameters
    x0 = []
    for i in range(6):
        for j in range(i+1):
            x0.append(initial_initial_covariance[i,j])
    for i in range(3):
        for j in range(i+1):
            x0.append(initial_measurement_noise[i,j])
    for i in range(6):
        for j in range(i+1):
            x0.append(initial_process_noise[i,j])

    bayes_opt = BayesianOptimizer(
        search_space=search_space,
        objective_function=objective_fn,
        # x0=x0,
        n_calls=15,
        n_initial_points=5,
        batch_size=5,
        random_state=42
    )

    # 5. Run the optimization
    print("Starting Bayesian Optimization...")
    results = bayes_opt.optimize()
    print("Optimization completed.")

    # 6. Extract the best parameters
    best_params = results.x
    best_score = results.fun
    print(f"Best Score: {best_score}")
    print(f"Best Parameters: {best_params}")

    # 7. Apply the best parameters to the Kalman Filter and visualize
    # Reconstruct P, R, Q from best_params
    P_params = best_params[:21]
    R_params = best_params[21:27]
    Q_params = best_params[27:48]

    P = np.zeros((6,6))
    Q = np.zeros((6,6))
    R = np.zeros((3,3))

    P_indices = np.tril_indices(6)
    P[P_indices] = P_params

    R_indices = np.tril_indices(3)
    R[R_indices] = R_params

    Q_indices = np.tril_indices(6)
    Q[Q_indices] = Q_params

    # Ensure positive definiteness
    epsilon = 1e-6
    P += epsilon * np.eye(6)
    Q += epsilon * np.eye(6)

    # Initialize the Kalman Filter with best parameters
    initial_state = np.zeros((6,1))
    control_input = np.zeros((3,1))

    kf = AvionicsKalmanFilter(
        initial_state=initial_state,
        initial_covariance=P,
        control_input=control_input,
        process_noise=Q,
        measurement_noise=R
    )

    # 8. (Optional) Re-run the filter with the best parameters and visualize results
    # For demonstration, let's assume using the first real dataset
    if real_dataset_files:
        file_path = real_dataset_files[0]


        rocket = Rocket(
            motorAccel=125, 
            burnTime=2.5, 
            dragCoef=0.5, 
            topCrossSectionalArea=0.07296, 
            sideCrossSectionalArea=0.55741824,          # 6 ft^2
            mass=40.8233)                               # 90 kg
        
        loader = RocketDataGenerator(
            rocket=rocket, 
            loop_frequency=10, 
            pre_launch_delay=10,
            launch_angle=0,
            wind_affector=None
        )

        data_dict = loader.generate()

        time_data = data_dict["time"]
        r_x, r_y, r_z, v_x, v_y, v_z, a_x, a_y, a_z = None, None, None, None, None, None, None, None, None

        m_r_x = data_dict["r_x"]
        m_r_y = data_dict["r_y"]
        m_r_z = data_dict["r_z"]
        m_a_x = data_dict["a_x"]
        m_a_y = data_dict["a_y"]
        m_a_z = data_dict["a_z"]

        measured_positions = []
        measured_accelerations = []

        # make measured position and accelerations a list of 3D numpy arrays
        for i in range(len(time_data)):
            measured_positions.append(np.array([[m_r_x[i]], [m_r_y[i]], [m_r_z[i]]]).reshape(-1,1))   
            measured_accelerations.append(np.array([[m_a_x[i]], [m_a_y[i]], [m_a_z[i]]]).reshape(-1,1))

        # Reset Kalman Filter metrics
        kf.reset_metrics()

        # Run the filter
        estimated_positions = []
        estimated_velocities = []
        estimated_times = []
        for i in range(1, len(time_data)):
            dt = time_data[i] - time_data[i-1]

            measurement = measured_positions[i]
            ctrl = measured_accelerations[i]

            kf.iterate(dt, measurement, ctrl, true_state=None)

            estimated_positions.append(kf.get_state()[:3].flatten())
            estimated_velocities.append(kf.get_state()[3:].flatten())
            estimated_times.append(time_data[i])

        # Prepare data for plotting
        data = {
            "time": time_data,
            "r_x": r_x,
            "r_y": r_y,
            "r_z": r_z,
            "measured_r_x": [x[0] for x in measured_positions],
            "measured_r_y": [y[1] for y in measured_positions],
            "measured_r_z": [z[2] for z in measured_positions],
            "estimated_r_x": [x[0] for x in estimated_positions],
            "estimated_r_y": [y[1] for y in estimated_positions],
            "estimated_r_z": [z[2] for z in estimated_positions],
            "v_x": v_x,
            "v_y": v_y,
            "v_z": v_z,
            "estimated_v_x": [vx[0] for vx in estimated_velocities],
            "estimated_v_y": [vy[1] for vy in estimated_velocities],
            "estimated_v_z": [vz[2] for vz in estimated_velocities],
            "a_x": a_x,
            "a_y": a_y,
            "a_z": a_z,
            "measured_a_x": [x[0] for x in measured_accelerations],
            "measured_a_y": [y[1] for y in measured_accelerations],
            "measured_a_z": [z[2] for z in measured_accelerations]
        }

        manager = pm.PlotManager(data)

        manager.add_plot("z_position", lambda: pm.plot_z_position(manager))
        manager.add_plot("xyz_position", lambda: pm.plot_xyz_position(manager))
        manager.add_plot("z_velocity", lambda: pm.plot_z_velocity(manager))

        # Show all plots
        manager.show_all()

if __name__ == "__main__":
    main()
