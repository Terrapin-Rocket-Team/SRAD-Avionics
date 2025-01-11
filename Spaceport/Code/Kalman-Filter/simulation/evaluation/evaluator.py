# evaluation/evaluator.py
import numpy as np
from evaluation.metrics import compute_nis, compute_nees, compute_regularization
from optimization.parameter_space import build_matrix_from_lower_triangle

def evaluate_parameters(x, param_space, real_datasets, synthetic_datasets, rocket_config_sampler, kf_class, **kwargs):
    """
    :param x: 1D array containing the candidate parameters
    :param param_space: ParameterSpace object to decode x -> P0, R, Q
    :param real_datasets: list of real dataset filenames or pre-loaded data
    :param synthetic_datasets: placeholders for how many synthetic datasets we want
    :param rocket_config_sampler: function that returns random rocket parameters
    :param kf_class: e.g. MyLinearKalmanFilter
    :returns: scalar cost to minimize
    """
    p0_part, r_part, q_part = param_space.split_params(x)
    
    P0 = build_matrix_from_lower_triangle(p0_part, 6)
    R_lower = build_matrix_from_lower_triangle(r_part, 3)
    # For R, you might do R = R_lower @ R_lower.T (to ensure positive-definite)
    R = R_lower @ R_lower.T

    # Q could be a matrix or scalar:
    if param_space.q_lower_dim == 21:
        Q_lower = build_matrix_from_lower_triangle(q_part, 6)
        Q = Q_lower @ Q_lower.T
    else:
        # single scalar
        Q = float(q_part[0])

    # Combine for cost
    total_error = 0.0
    count = 0

    # 1. Evaluate on real datasets (NIS-based)
    for real_data in real_datasets:
        data_dict = load_real_data(real_data)  # or passed in directly
        # run_kf -> returns average or sum of NIS
        # because we only have measurements, not true states
        nis_score = run_kf_on_real_data(data_dict, kf_class, P0, R, Q, **kwargs)
        total_error += nis_score
        count += 1

    # 2. Evaluate on synthetic datasets (NIS + NEES)
    for _ in range(len(synthetic_datasets)):
        # sample rocket config from user-defined distributions:
        rocket_params = rocket_config_sampler()  
        data_dict = generate_synthetic_data(rocket_params)
        results = run_kf_on_synthetic_data(data_dict, kf_class, P0, R, Q, **kwargs)
        # results might be dict with "avg_nis", "avg_nees"
        total_error += results["avg_nis"] + results["avg_nees"]
        count += 1

    # Add regularization
    reg = compute_regularization(x)

    # Return average or sum
    return (total_error / max(1, count)) + reg
