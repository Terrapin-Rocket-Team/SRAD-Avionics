from scipy.interpolate import CubicSpline
import numpy as np
import matplotlib.pyplot as plt
from optimizers.objective import generate_random_spline

x = np.linspace(0, 1000, 1000)  # Input range
y = generate_random_spline(-1, 1)

y_graph = y(x)

print(y(15))

# Plot the function
plt.plot(x, y_graph)
plt.title("Random Smooth Function (Cubic Spline) within Bounds")
plt.xlabel("x")
plt.ylabel("y")
plt.ylim(-1.2, 1.2)
plt.show()
