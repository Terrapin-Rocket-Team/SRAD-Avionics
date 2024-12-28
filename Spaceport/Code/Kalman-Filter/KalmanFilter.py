import numpy as np

class LinearKalmanFilter:
    def __init__(self, X, P, U):
        # Constructor initializes the matrices and vectors
        if not (isinstance(U, np.ndarray) and isinstance(X, np.ndarray) and isinstance(P, np.ndarray)):
            raise ValueError('Incorrect argument types for U, X, or P')

        self.u = U  # Control Vector
        self.x = X  # State Vector
        self.p = P  # Estimate Covariance Matrix
        self.f = None  # Placeholder for State Transition Matrix
        self.g = None  # Placeholder for Control Matrix
        self.h = None  # Placeholder for Observation Matrix
        self.k = None  # Kalman Gain
        self.r = None  # Measurement Uncertainty Matrix
        self.q = None  # Process Noise Matrix
        self.c = None  # Drag thing

        self.meas_uncertainty = 0.5
        self.process_noise = 5

    def predictState(self):
        # Predict the next state
        # print(self.f)
        # print(self.x)
        # print(self.g)
        # print(self.u)
        self.x = (self.f @ self.x) + (self.g @ self.u)
        return self.x

    def estimateState(self, measurement):
        # Estimate the state based on measurement
        self.x = self.x + (self.k @ (measurement - (self.h @ self.x)))
        return self.x

    def calculateKalmanGain(self):
        # Calculate Kalman Gain
        self.k = self.p @ self.h.T @ np.linalg.inv((self.h @ self.p @ self.h.T) + self.r)
        return self.k

    def covarianceUpdate(self):
        # Update the covariance matrix
        n = 6
        self.p = (np.eye(n) - self.k @ self.h) @ self.p @ (np.eye(n) - self.k @ self.h).T + self.k @ self.r @ self.k.T
        return self.p

    def covarianceExtrapolate(self):
        # Extrapolate the covariance matrix
        self.p = self.f @ self.p @ self.f.T + self.q
        return self.p

    def calculateInitialValues(self, dt):
        # Calculate initial values for F, G, Q, etc.
        self.f = np.array([[1, 0, 0, dt, 0, 0],
                           [0, 1, 0, 0, dt, 0],
                           [0, 0, 1, 0, 0, dt],
                           [0, 0, 0, 1, 0,  0],
                           [0, 0, 0, 0, 1,  0],
                           [0, 0, 0, 0, 0,  1]])

        self.g = np.array([[0.5 * dt**2, 0, 0],
                           [0, 0.5 * dt**2, 0],
                           [0, 0, 0.5 * dt**2],
                           [dt, 0, 0],
                           [0, dt, 0],
                           [0, 0, dt]])

        self.q = self.g @ (self.process_noise**2) @ self.g.T

        self.predictState()
        self.covarianceExtrapolate()

    def iterate(self, dt, measurement, control):
        # Update step
        self.u = control  # Update control vector
        z = measurement  # Measurement vector

        self.f = np.array([[1, 0, 0, dt, 0, 0],
                           [0, 1, 0, 0, dt, 0],
                           [0, 0, 1, 0, 0, dt],
                           [0, 0, 0, 1, 0,  0],
                           [0, 0, 0, 0, 1,  0],
                           [0, 0, 0, 0, 0,  1]])

        self.g = np.array([[0.5 * dt**2, 0, 0],
                           [0, 0.5 * dt**2, 0],
                           [0, 0, 0.5 * dt**2],
                           [dt, 0, 0],
                           [0, dt, 0],
                           [0, 0, dt]])

        self.q = (self.process_noise**2) * self.g @ self.g.T
        self.r = np.eye(3) * self.meas_uncertainty
        self.h = np.array([[1, 0, 0, 0, 0, 0],
                           [0, 1, 0, 0, 0, 0],
                           [0, 0, 1, 0, 0, 0]])

        self.calculateKalmanGain()
        self.estimateState(measurement)
        self.covarianceUpdate()

        # Predict step
        self.predictState()
        self.covarianceExtrapolate()
        return self
