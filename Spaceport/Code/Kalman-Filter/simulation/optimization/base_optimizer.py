# optimization/base_optimizer.py
import abc

class BaseOptimizer(abc.ABC):
    """
    Abstract base class for any optimizer.
    """

    @abc.abstractmethod
    def optimize(self, objective_fn, param_space, n_iterations=50, **kwargs):
        """
        :param objective_fn: a callable that takes a parameter vector x and returns a cost
        :param param_space:  a ParameterSpace object or similar
        :param n_iterations: how many optimization iterations
        :return: best_params, best_cost
        """
        pass
