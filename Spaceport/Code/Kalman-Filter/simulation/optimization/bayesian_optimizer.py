import numpy as np
from .base_optimizer import BaseOptimizer

from skopt import gp_minimize
from skopt.space import Real

class BayesianOptimizer(BaseOptimizer):

    def optimize(self, objective_fn, param_space, n_iterations=50, **kwargs):

        # Convert param_space bounds to scikit-optimize space
        space = [Real(l, h, name=f"x{i}") for i, (l,h) in enumerate(param_space.p0_bounds + param_space.r_bounds + param_space.q_bounds)]

        result = gp_minimize(
            func=objective_fn,
            dimensions=space,
            n_calls=n_iterations,
            random_state=42,
            **kwargs
        )
        
        best_params = result.x
        best_cost = result.fun
        
        return best_params, best_cost

    