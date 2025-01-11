# optimizers/bayesian_optimizer.py

from .base_optimizer import BaseOptimizer
from skopt import gp_minimize
from skopt.space import Real, Integer, Categorical
from skopt.utils import use_named_args
import numpy as np
import itertools

class BayesianOptimizer(BaseOptimizer):
    """
    Bayesian Optimizer using scikit-optimize's gp_minimize.
    """

    def __init__(self, 
                 search_space, 
                 objective_function, 
                 n_calls=50, 
                 n_initial_points=10, 
                 batch_size=5,
                 random_state=42):
        """
        :param search_space: List of skopt.space.Dimension objects defining the search space.
        :param objective_function: Function to minimize. Should accept a list of parameters.
        :param n_calls: Total number of evaluations.
        :param n_initial_points: Number of initial random evaluations.
        :param batch_size: Number of evaluations per batch.
        :param random_state: Seed for reproducibility.
        """
        self.search_space = search_space
        self.objective_function = objective_function
        self.n_calls = n_calls
        self.n_initial_points = n_initial_points
        self.batch_size = batch_size
        self.random_state = random_state
        self.results = None

    def optimize(self):
        """
        Run the Bayesian optimization.
        """
        self.results = gp_minimize(
            func=self.objective_function,
            dimensions=self.search_space,
            acq_func='EI',  # Expected Improvement
            n_calls=self.n_calls,
            n_initial_points=self.n_initial_points,
            random_state=self.random_state,
            verbose=True,
            callback=None,
            n_jobs=-1  # Parallel evaluations
        )
        return self.results
