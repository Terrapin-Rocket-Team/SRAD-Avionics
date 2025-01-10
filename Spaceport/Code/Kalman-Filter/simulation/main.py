
import numpy as np
import matplotlib.pyplot as plt
import plot_manager as pm

# Import your classes
from models.rocket import Rocket
from data_generators.rocket_data_generator import RocketDataGenerator
from data_generators.real_flight_data_loader import RealFlightDataLoader

from kalman_filters.my_linear_kalman_filter import MyLinearKalmanFilter

from noise_generators.gaussian_noise_generator import GaussianNoiseGenerator
# from noise_generators.pink_noise_generator import PinkNoiseGenerator
# from noise_generators.drift_noise_generator import DriftNoiseGenerator

from sensors.sensor import Sensor
# from sensors.acceleration_sensor import AccelerationSensor

def main():
    # 1. Choose data source: simulated or real
    use_simulated = False  # set to False to load real CSV

    measured_positions = []
    measured_accelerations = []

    if use_simulated:
        rocket = Rocket(motorAccel=125, burnTime=2.5, dragCoef=0.5, crossSectionalArea=0.07296)
        generator = RocketDataGenerator(
            rocket=rocket, 
            loop_frequency=50, 
            pre_launch_delay=1.0  # 1 second on the pad
        )
        data_dict = generator.generate()

        # save everything as ground truth
        time_data = data_dict["time"]
        r_x = data_dict["r_x"]
        r_y = data_dict["r_y"]
        r_z = data_dict["r_z"]
        v_x = data_dict["v_x"]
        v_y = data_dict["v_y"]
        v_z = data_dict["v_z"]
        a_x = data_dict["a_x"]
        a_y = data_dict["a_y"]
        a_z = data_dict["a_z"]
    else:
        # Real data from CSV
        loader = RealFlightDataLoader(
            file_path="flight_data/2024_SAC_Flight_Data.csv",
            rescale_factor=0.3,
            pre_launch_cut=2.0
        )
        data_dict = loader.generate()

        # real data is already noisy and we have no ground truth
        time_data = data_dict["time"]
        r_x, r_y, r_z, v_x, v_y, v_z, a_x, a_y, a_z = None

        measured_positions = np.array([
                                       data_dict["r_x"], 
                                       data_dict["r_y"], 
                                       data_dict["r_z"]]).T
        measured_accelerations = np.array([
                                       data_dict["a_x"], 
                                       data_dict["a_y"], 
                                       data_dict["a_z"]]).T

    # 2. if simulated, add noise to the data to get "measured" data
    if use_simulated:

        gps_noise = 1.0
        position_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=1.0)])

        imu_noise = 0.1
        acceleration_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=0.1)])

        for i in range(len(time_data)):
            pos_measured = position_sensor.measure(np.array([r_x[i], r_y[i], r_z[i]]))
            measured_positions.append(pos_measured)

            a_measured = acceleration_sensor.measure(np.array([a_x[i], a_y[i], a_z[i]]))
            measured_accelerations.append(a_measured)

    # 3. Set up the Kalman filter
    #   A typical initial covariance
    initial_covariance = 500 * np.eye(6)
    #   An initial state (6x1)
    initial_state = np.zeros((6,1))
    #   A control vector (acceleration)
    control_input = np.zeros((3,1))

    kf = MyLinearKalmanFilter(
        initial_state=initial_state,
        initial_covariance=initial_covariance,
        control_input=control_input,
        measurement_noise=0.5 * np.eye(3),  # Example
        process_noise=5.0       # Example
    )

    # 4. Run the filter in a loop
    estimated_positions = []
    estimated_velocities = []
    estimated_times = []

    for i in range(1, len(time_data)):
        dt = time_data[i] - time_data[i-1]
        
        # measurement is 3x1: x, y, z
        measurement = measured_positions[i]

        # Let’s approximate the control from a_x, a_y, a_z (the “true” or “guessed” rocket acceleration)
        # Typically, you'd also apply a noise generator for an "accel sensor".
        # For now we just use the raw a_x,y,z - gravity is in there, so we might do (a_z[i] - 9.81) etc.
        ctrl = np.array([[a_x[i]], [a_y[i]], [a_z[i]]])

        # One iteration
        kf.iterate(dt, measurement, ctrl)
        estimated_positions.append(kf.get_state()[:3])  # x, y, z from the filter
        estimated_velocities.append(kf.get_state()[3:])  # vx, vy, vz from the filter
        estimated_times.append(time_data[i])

    # 5. Plot results
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
