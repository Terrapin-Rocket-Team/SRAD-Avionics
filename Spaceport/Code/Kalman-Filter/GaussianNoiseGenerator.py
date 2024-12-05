#GaussianNoiseGenerator.py
import numpy as np
def GaussianNoiseGenerator(data,sigma):
    noise = sigma*np.random.randn((data.shape));
    noisy_data = data + noise;
    return noisy_data

