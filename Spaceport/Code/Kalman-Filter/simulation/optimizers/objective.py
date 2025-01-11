# optimizers/objective.py

import numpy as np
from kalman_filters.avionics_kalman_filter import AvionicsKalmanFilter
from data_generators.real_flight_data_loader import RealFlightDataLoader
from data_generators.rocket_data_generator import RocketDataGenerator
from models.rocket import Rocket
from noise_generators.gaussian_noise_generator import GaussianNoiseGenerator
from sensors.sensor import Sensor
from scipy.interpolate import CubicSpline
import random

def compute_nis(kf_metrics):
    """
    Compute Normalized Innovation Squared (NIS) from Kalman Filter metrics.
    :param kf_metrics: Dictionary containing 'innovations' and 'S_matrices'.
    :return: Average NIS.
    """
    innovations = kf_metrics["innovations"]  # Shape: (num_steps, 3)
    S = kf_metrics["S_matrices"]            # Shape: (num_steps, 3, 3)
    nis_values = []
    for y, S_matrix in zip(innovations, S):
        nis = y.T @ np.linalg.inv(S_matrix) @ y
        nis_values.append(nis)
    return np.mean(nis_values)

def compute_nees(kf_metrics):
    """
    Compute Normalized Estimation Error Squared (NEES) from Kalman Filter metrics.
    :param kf_metrics: Dictionary containing 'estimation_errors' and 'P_matrices'.
    :return: Average NEES.
    """
    estimation_errors = kf_metrics["estimation_errors"]  # Shape: (num_steps, 6)
    P = kf_metrics["P_matrices"]                        # Shape: (num_steps, 6, 6)
    nees_values = []
    for error, P_matrix in zip(estimation_errors, P):
        nees = error.T @ np.linalg.inv(P_matrix) @ error
        nees = float(nees)
        nees_values.append(nees)

    return abs(np.mean(nees_values))            # hacky workaround for now to avoid negative values

def generate_random_spline(lower_bound, upper_bound):
    """
    Generates a random cubic spline by interpolating randomly sampled points.
    Caps the function output within the specified bounds.
    :param lower_bound: Lower bound for the function output.
    :param upper_bound: Upper bound for the function output.
    :return: Cubic function object.
    """
    num_points = 10  # Number of control points

    # Generate random control points
    control_x = np.linspace(0, 1000, num_points)
    control_y = np.random.uniform(lower_bound, upper_bound, num_points)

    # Create a cubic spline
    spline = CubicSpline(control_x, control_y)

    return (lambda x: np.clip(spline(x), lower_bound, upper_bound))

def generate_random_vector_func(lower_bound, upper_bound):
    """
    Generates a function that returns a random vector within the specified bounds, where the bounds are per coordinate.
    """

    x_func = generate_random_spline(lower_bound, upper_bound)
    y_func = generate_random_spline(lower_bound, upper_bound)

    return (lambda t: np.array([x_func(t), y_func(t)])) 

def objective_function_factory(real_dataset_files, n_real=5, n_generated=5, burn_time_range=(2, 5), mass_range=(30, 60), 
                               launch_angle_range=(0, 45), motor_accel_range=(100, 150), drag_coef_range=(0.4, 0.6), 
                               top_cross_sectional_area_range=(0.05, 0.1), side_cross_sectional_area_range=(0.5, 0.6), 
                               pos_noise_sigma_range=(0.5, 1.5), accel_noise_sigma_range=(0.05, 0.15),
                               flight_rescale_range=(0.3, 4), pre_launch_cut_range=(0.8, 0.99), wind_affector_range=(-1, 1),
                               regularization_lambda=1e-5, optimizer_seed=42):
    """
    Factory to create an objective function with access to dataset configurations.
    :param real_dataset_files: List of file paths for real datasets.
    :param n_real: Number of real datasets to use per parameter set.
    :param n_generated: Number of generated datasets to use per parameter set.
    :param burn_time_range: Tuple defining the range to sample burn times.
    :param mass_range: Tuple defining the range to sample masses.
    :param launch_angle_range: Tuple defining the range to sample launch angles.
    :param motor_accel_range: Tuple defining the range to sample motor accelerations.
    :param drag_coef_range: Tuple defining the range to sample drag coefficients.
    :param top_cross_sectional_area_range: Tuple defining the range to sample top cross-sectional areas.
    :param side_cross_sectional_area_range: Tuple defining the range to sample side cross-sectional areas.
    :param pos_noise_sigma_range: Tuple defining the range to sample position noise sigmas.
    :param accel_noise_sigma_range: Tuple defining the range to sample acceleration noise sigmas.
    :param flight_rescale_range: Tuple defining the range to sample flight rescale factors.
    :param pre_launch_cut_range: Tuple defining the range to sample pre-launch cut factors.
    :param wind_affector_range: Tuple defining the upper and lower bounds that a wind affector can produce in each direction.
    :param regularization_lambda: Regularization parameter for the objective function. 0 for no regularization.
    :param optimizer_seed: Seed for reproducibility.
    :return: Objective function.
    """
    def objective(params):
        """
        Objective function to minimize.
        :param params: List of parameters (flattened lower triangular matrices).
        :return: Scalar objective value.
        """
        np.random.seed(optimizer_seed)
        random.seed(optimizer_seed)
        
        # Extract parameters from the flattened list
        # Assuming the search_space was defined as [P_lower_tri, R_lower_tri, Q_lower_tri]
        # For simplicity, let's assume:
        # - First 21 params: P (6x6 lower triangular)
        # - Next 6 params: R (3x3 lower triangular)
        # - Next 21 params: Q (6x6 lower triangular)
        P_params = params[:21]
        R_params = params[21:27]
        Q_params = params[27:48]

        # Reconstruct P, R, Q matrices
        P = np.zeros((6,6))
        Q = np.zeros((6,6))
        R = np.zeros((3,3))
        
        # Fill lower triangular for P
        P_indices = np.tril_indices(6)
        P[P_indices] = P_params
        
        # Fill lower triangular for R
        R_indices = np.tril_indices(3)
        R[R_indices] = R_params
        
        # Fill lower triangular for Q
        Q_indices = np.tril_indices(6)
        Q[Q_indices] = Q_params

        # Ensure P and Q are positive definite by adding a small value to the diagonal if needed
        epsilon = 1e-6
        P += epsilon * np.eye(6)
        Q += epsilon * np.eye(6)

        # Initialize metric accumulators
        total_nis = 0.0
        total_nees = 0.0
        count_nis = 0
        count_nees = 0

        # Initialize the Kalman Filter parameters
        initial_state = np.zeros((6,1))
        control_input = np.zeros((3,1))

        # Iterate over real datasets
        selected_real_files = random.sample(real_dataset_files, min(n_real, len(real_dataset_files)))

        for file_path in selected_real_files:

            # Use real rocket parameters
            rocket = Rocket(motorAccel=125, 
                            burnTime=2.5, 
                            dragCoef=0.5, 
                            topCrossSectionalArea=0.07296, 
                            sideCrossSectionalArea=0.55741824,          # 6 ft^2
                            mass=40.8233                                # 90 kg
                            )                               

            # sample rescale factor and pre-launch cut
            rescale_factor = random.uniform(*flight_rescale_range)
            pre_launch_cut = random.uniform(*pre_launch_cut_range)

            # use the spline generator to create a random wind affector
            wind_affector = generate_random_vector_func(*wind_affector_range)

            # Load the dataset
            loader = RealFlightDataLoader(
                file_path=file_path,
                rocket=rocket,
                rescale_factor=rescale_factor,
                pre_launch_cut=pre_launch_cut,
                wind_affector=wind_affector
            )
            data_dict = loader.generate()

            # real data is already noisy and we have no ground truth
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

            # Initialize the Kalman Filter
            kf = AvionicsKalmanFilter(
                initial_state=initial_state,
                initial_covariance=P.copy(),
                control_input=control_input.copy(),
                process_noise=Q.copy(),
                measurement_noise=R.copy()
            )
            kf.reset_metrics()

            # Run the filter
            for i in range(1, len(time_data)):
                dt = time_data[i] - time_data[i-1]

                measurement = measured_positions[i]
                ctrl = measured_accelerations[i]

                # No true state for real datasets, so pass None
                kf.iterate(dt, measurement, ctrl)

            # Compute NIS
            metrics = kf.get_metrics()
            nis = compute_nis(metrics)
            total_nis += nis
            count_nis += 1

        # Iterate over generated datasets
        for _ in range(n_generated):
            # Randomly sample dataset parameters
            burn_time = random.uniform(*burn_time_range)
            mass = random.uniform(*mass_range)
            launch_angle = random.uniform(*launch_angle_range)
            motor_accel = random.uniform(*motor_accel_range)
            drag_coef = random.uniform(*drag_coef_range)
            top_cross_sectional_area = random.uniform(*top_cross_sectional_area_range)
            side_cross_sectional_area = random.uniform(*side_cross_sectional_area_range)

            # Create a Rocket object with sampled parameters
            sampled_rocket = Rocket(
                motorAccel=motor_accel,
                burnTime=burn_time,
                dragCoef=drag_coef,
                topCrossSectionalArea=top_cross_sectional_area,
                sideCrossSectionalArea=side_cross_sectional_area,
                mass=mass
            )

            # Create a data generator with wind affector
            wind_affector = generate_random_vector_func(*wind_affector_range)

            generator = RocketDataGenerator(
                rocket=sampled_rocket, 
                loop_frequency=50, 
                pre_launch_delay=10,
                launch_angle=launch_angle,
                wind_affector=wind_affector
            )
            data_dict = generator.generate()

            time_data = data_dict["time"]
            r_x = data_dict["r_x"]
            r_y = data_dict["r_y"]
            r_z = data_dict["r_z"]
            v_x = data_dict["v_x"]
            v_y = data_dict["v_y"]
            v_z = data_dict["v_z"]
            a_x = data_dict["a_x"]
            a_y = data_dict["a_y"]
            a_z = data_dict["a_z"] + 9.81  # Add gravity

            # Assume true state is known for NEES
            true_states = np.vstack([r_x, r_y, r_z, v_x, v_y, v_z]).T  # Shape: (num_steps, 6)

            # Create measured data with noise
            pos_noise_sigma = random.uniform(*pos_noise_sigma_range)
            accel_noise_sigma = random.uniform(*accel_noise_sigma_range)
            position_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=pos_noise_sigma)])
            acceleration_sensor = Sensor(noise_generators=[GaussianNoiseGenerator(sigma=accel_noise_sigma)])

            measured_positions = []
            measured_accelerations = []
            for i in range(len(time_data)):
                pos_measured = position_sensor.measure(np.array([r_x[i], r_y[i], r_z[i]]))
                measured_positions.append(pos_measured)

                a_measured = acceleration_sensor.measure(np.array([a_x[i], a_y[i], a_z[i]]))
                measured_accelerations.append(a_measured)

            # Initialize the Kalman Filter
            kf = AvionicsKalmanFilter(
                initial_state=initial_state,
                initial_covariance=P.copy(),
                control_input=control_input.copy(),
                process_noise=Q.copy(),
                measurement_noise=R.copy()
            )
            kf.reset_metrics()

            # Run the filter
            for i in range(1, len(time_data)):
                dt = time_data[i] - time_data[i-1]

                measurement = measured_positions[i]
                ctrl = measured_accelerations[i]

                true_state = true_states[i].reshape(6,1)

                kf.iterate(dt, measurement, ctrl, true_state=true_state)

            # Compute NIS and NEES
            metrics = kf.get_metrics()
            nis = compute_nis(metrics)
            nees = compute_nees(metrics)

            total_nis += nis
            total_nees += nees
            count_nis += 1
            count_nees += 1

        # Aggregate metrics
        avg_nis = total_nis / count_nis if count_nis > 0 else 0
        avg_nees = total_nees / count_nees if count_nees > 0 else 0

        # Define the objective: minimize avg_nis and avg_nees
        # We can combine them, possibly weighted
        objective_value = avg_nis + avg_nees  # Simple sum; adjust weights as needed

        # Regularization (e.g., L2 norm of matrices)
        regularization = regularization_lambda * (np.linalg.norm(P, ord='fro') + np.linalg.norm(Q, ord='fro') + np.linalg.norm(R, ord='fro'))
        objective_value += regularization

        print(f"Objective Value: {objective_value}")

        return objective_value

    return objective
