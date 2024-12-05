# KalmanFilter.py
import numpy as np
class LinearKalmanFilter:
    def __init__(self,F,G,U,X,H,P,R,K,Q,c,):
        self.f = F # State transition matrix
        self.g = G # Control Matrix
        self.u = U # Control Vector
        self.x = X # State Vector
        self.h = H # Observation Matrix
        self.p = P # Estimate Covariance Matrix
        self.r = R # Measurement uncertainty Matrix
        self.k = K # Kalman Gain
        self.q = Q # Process noise matrix
        self.C = c # Drag thing
        self.meas_uncertainty = .5;
        self.process_noise = 5;

    @property
    def LinearKalmanFilter(self,X,P,U):
        self.x = X; 
        self.p = P;
        self.u = U;
        return self.LinearKalmanFilter

    @property
    def predictState(self):
        self.x = (self.f*self.x) + (self.g*self.u)
        return self.x
    

    @property
    def estimateState(self,measurement):
        self.x = self.x + self.k*(measurement - self.h*self.x)
        return self.x

    
    @property
    def calculateKalmanGain(self):
        self.k = self.p * self.h * np.linalg.inv(self.h * self.p * np.transpose(self.h) + self.r);
        return self.k

    @property
    def covarianceUpdate(self):
        n = 6;
        self.p = np.transpose((np.eye(n) - self.k * self.h) * self.p * (np.eye(n) - self.k * self.h)) + self.k*self.r*np.transpose(self.k);
        return self.p  
    
    @property
    def covarianceExtrapolate(self):
        self.p = self.f * self. p * np.transpose(self.f) + self.q;
        return self.p

    @property 
    def calculateInitialValues(self, dt):
        self.F = np.array([1, 0, 0, dt, 0, 0],
                          [0, 1, 0, 0, dt, 0],
                          [0, 0, 1, 0, 0, dt],
                          [0, 0, 0, 1, 0,  0],
                          [0, 0, 0, 0, 1,  0],
                          [0, 0, 0, 0, 0,  1]);
        self.g = np.array([0.5*dt*dt, 0, 0]
                          [0, 0.5*dt*dt, 0],
                          [0, 0, 0.5*dt*dt],
                          [dt, 0,        0],
                          [0, dt,        0],
                          [0, 0,        dt]);

        self.q = self.g*self.process_noise*self.process_noise*np.transpose(self.g);
        
        self.predictState()
        
        self.covarianceExtrapolate()
        
       

    @property
    def iterate(self,dt,measurement,control):
        self.z = measurement;
        self.u = control;

        self.F = np.array([1, 0, 0, dt, 0, 0],
                          [0, 1, 0, 0, dt, 0],
                          [0, 0, 1, 0, 0, dt],
                          [0, 0, 0, 1, 0,  0],
                          [0, 0, 0, 0, 1,  0],
                          [0, 0, 0, 0, 0,  1]);
        self.g = np.array([0.5*dt*dt, 0, 0]
                          [0, 0.5*dt*dt, 0],
                          [0, 0, 0.5*dt*dt],
                          [dt, 0,        0],
                          [0, dt,        0],
                          [0, 0,        dt]);

        self.q = self.g*self.process_noise*self.process_noise*np.transpose(self.g);
        self.r = np.eye(3)*self.meas_uncertainty
        self.h = np.array([1, 0, 0, 0, 0, 0],
                          [0, 1, 0, 0, 0, 0],
                          [0, 0, 1, 0, 0, 0])

        self.calculateKalmanGain()           
        self.estimateState()         
        self.covarianceUpdate()
        
        #predict step

        self.predictState()          
        self.covarianceExtrapolate()
    