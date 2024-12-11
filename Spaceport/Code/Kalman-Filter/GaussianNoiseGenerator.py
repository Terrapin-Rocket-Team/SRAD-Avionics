#GaussianNoiseGenerator.py
import numpy as np
def GaussianNoiseGenerator(data,sigma):
    data = np.array(data)
    noise = sigma*np.random.randn(len(data))
    noisy_data = np.array(data) + noise
    return noisy_data

