
import numpy as np 
import matplotlib.pyplot as plt
import plot_managers.plot_manager as pm

# Import your classes
from models.new_rocket import NewRocket
from data_generators.real_flight_data_loader import RealFlightDataLoader
from data_generators.extended_rocket_data_generator import EnhancedRocketDataGenerator

from kalman_filters.extended_kalman_filter import RocketEKF

from noise_generators.gaussian_noise_generator import GaussianNoiseGenerator
# from noise_generators.pink_noise_generator import PinkNoiseGenerator
# from noise_generators.drift_noise_generator import DriftNoiseGenerator

from sensors.sensor import Sensor
# from sensors.acceleration_sensor import AccelerationSensor

def main():
    # 1. Choose data source: simulated
    use_simulated = True  # set to False to load real CSV

    measured_gps_positions = []
    measured_barometer_positions = []
    measured_accelerations = []
    measured_quaternions = []

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

    generator = EnhancedRocketDataGenerator(
        rocket=rocket, 
        loop_frequency=50, 
        pre_launch_delay=10,
        launch_angle=1,
        wind_affector=(lambda t: np.array([0*np.sin(t), 0*np.cos(t), 0]))
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
    a_z = data_dict["a_z"] # In the future add gravity (9.81) since IMU doesn't account for it
    quat = data_dict["quaternion"]

    # 2. if simulated, add noise to the data to get "measured" data
    if use_simulated:
        # in the future need to get actual sensor noise specifications from datasheet
        gps_noise = 1.0
        position_gps_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=gps_noise)])

        barometer_noise = .3
        position_barometer_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=barometer_noise)])

        imu_noise = 0.1
        acceleration_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=imu_noise)])
        # Realistically, quaternions are taken from the complementary filter
        quaternion_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=imu_noise)]) 

        for i in range(len(time_data)):
            pos_gps_measured = position_gps_sensor.measure(np.array([r_x[i], r_y[i], r_z[i]]))
            measured_gps_positions.append(pos_gps_measured)

            pos_barometer_measured = position_barometer_sensor.measure(np.array([r_x[i], r_y[i], r_z[i]]))
            measured_barometer_positions.append(pos_barometer_measured)

            a_measured = acceleration_sensor.measure(np.array([a_x[i], a_y[i], a_z[i]]))
            measured_accelerations.append(a_measured)

            q_measured = quaternion_sensor.measure(np.array([quat[i][0], quat[i][1], quat[i][2], quat[i][3]]))
            measured_quaternions.append(q_measured)

    # 3. Set up the Kalman filter
    #   A typical initial covariance
    initial_covariance = 500 * np.eye(6)
    #   An initial state (6x1)
    initial_state = np.zeros((6,1))

    # Construct R matrix
    R = np.diag([gps_noise**2, gps_noise**2, gps_noise**2, barometer_noise**2, imu_noise**2, imu_noise**2, imu_noise**2]) # Needs to be tuned

    # Construct Q Matrix
    Q = np.diag([0.5, 0.5, 0.5, 0.05, 0.05, 0.05])  # Needs to be tuned


    kf = RocketEKF(
        rocket=rocket,
        initial_state=initial_state,
        initial_covariance=initial_covariance,
        measurement_noise=R,
        process_noise=Q       
    )

    # 4. Run the filter in a loop
    estimated_positions = []
    estimated_velocities = []
    estimated_times = []

    for i in range(1, len(time_data)):
        dt = time_data[i] - time_data[i-1]
        
        # measurement is 7x1:
        measurement = np.array([measured_gps_positions[i][0], measured_gps_positions[i][1], measured_gps_positions[i][2],
                                 measured_barometer_positions[i][2], measured_accelerations[i][0], measured_accelerations[i][1], 
                                 measured_accelerations[i][2]])
        # quaternion from complementary filter
        quaternion = measured_quaternions[i]

        # One iteration
        kf.iterate(dt, measurement, quaternion)
        estimated_positions.append(kf.get_state()[:3])  # x, y, z from the filter
        estimated_velocities.append(kf.get_state()[3:])  # vx, vy, vz from the filter
        estimated_times.append(time_data[i])

    # 5. Plot results
    data = {
        "time": time_data,
        "r_x": r_x,
        "r_y": r_y,
        "r_z": r_z,
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
    manager.add_plot("mach_number", lambda: pm.plot_mach_number(manager))

    # Show all plots
    manager.show_all()

if __name__ == "__main__":
    main()
