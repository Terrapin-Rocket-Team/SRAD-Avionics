import numpy as np
from .base_optimizer import BaseOptimizer

class RandomSearch(BaseOptimizer):

    def optimize(self, objective_fn, param_space, n_iterations=50, **kwargs):

        # Just do random search as a placeholder:
        best_params = None
        best_cost = float("inf")
        all_bounds = param_space.p0_bounds + param_space.r_bounds + param_space.q_bounds
        for _ in range(n_iterations):
            # random sample in all_bounds
            candidate = []
            for (low, high) in all_bounds:
                candidate.append(np.random.uniform(low, high))
            candidate = np.array(candidate)
            cost = objective_fn(candidate)
            if cost < best_cost:
                best_cost = cost
                best_params = candidate
        return best_params, best_cost
