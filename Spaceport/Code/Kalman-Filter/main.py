from rocket_module import*
from KalmanFilter import*
from GaussianNoiseGenerator import*
from DataGenerator import*
import csv
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from enum import Enum

class DataType(Enum):
    Mock = 1
    OpenRocket = 2
    Flight = 3

# Setting sensor gaussian noise
accelNoise = .1
baroNoise = .5
gpsNoise = 1
g = 9.81

# Hardcode dataType to be one of the following

# Mock - 1
# OpenRocket - 2
# FlightData - 3

Mock = DataType.Mock.value
OpenRocket = DataType.OpenRocket.value
FlightData = DataType.Flight.value

dataType = 1

if dataType == Mock:
     #  First create mock data
        DataGenerator(dataFileName, loopFrequency, rocket_instance)

     # Read and assign mock data   
        df = pd.read_csv('mock_data.csv')

        data_Time = [df['t'].tolist()]
      
        data_r_x = [df['r_x'].tolist()]
        data_r_y = [df['r_y'].tolist()]
        data_r_z = [df['r_z'].tolist()]

        data_v_x = [df['v_x'].tolist()]
        data_v_y = [df['v_y'].tolist()]
        data_v_z = [df['v_z'].tolist()]
        
        data_a_x = [df['a_x'].tolist()]
        data_a_y = [df['a_y'].tolist()]
        data_a_z = [df['a_z'].tolist()]


if dataType == OpenRocket:       
      df = pd.read_csv('openrocket_2024_30k.csv')
      data_Time = [df['# Time (s)'].tolist()]
      
      data_r_x = [df['Position East of launch (m)'].tolist()]
      data_r_y = [df['Positions North of launch (m)'].tolist()]
      data_r_z = [df['Altitude (m)'].tolist()]

      # Diff poition to get velocity
      delta_T = np.diff(data_Time)
      delta_r_x = np.diff(data_r_x)
      delta_r_y = np.diff(data_r_y)
      
      data_v_x = [delta_r_x/delta_T]
      data_v_y = [delta_r_y/delta_T]
      data_v_z = [df['Vertical velocity (m/s)'].tolist()]
      
      # Diff velocity to get acceleration
      delta_v_x = np.diff(data_v_x)
      delta_v_y = np.diff(data_v_y)

      data_a_x = [delta_v_x/delta_T]
      data_a_y = [delta_v_y/delta_T]
      data_a_z = [df['Vertical Acceleration'].tolist()]      

# Read in Flight Data File
if dataType == FlightData:
      df = pd.read_csv('TADPOL_April_NY_post_processed2.csv')
      data_Time = [df['Time (ms)'].tolist()]
      
      data_r_x = [df['PosX (m)'].tolist()]
      data_r_y = [df['PosY (m)'].tolist()]
      data_r_z = [df['PosZ (m)'].tolist()]

      data_v_x = [df['VeloX (m/s)'].tolist()]
      data_v_y = [df['VeloY (m/s)'].tolist()]
      data_v_z = [df['VeloZ (m/s)'].tolist()]
        
      data_a_x = [df['AccelX (m/s^2)'].tolist()]
      data_a_y = [df['AccelY (m/s^2)'].tolist()]
      data_a_z = [df['AccelZ (m/s^2)'].tolist()]

# Add gaussian noise to generate measurement data
if dataType != FlightData:
      data_r_meas_x = GaussianNoiseGenerator(data_r_x, gpsNoise)
      data_r_meas_y = GaussianNoiseGenerator(data_r_y, gpsNoise)
      data_r_meas_z = GaussianNoiseGenerator(data_r_z, gpsNoise)
      data_a_meas_x = GaussianNoiseGenerator(data_a_x, gpsNoise)
      data_a_meas_y = GaussianNoiseGenerator(data_a_y, gpsNoise)
      data_a_meas_z = GaussianNoiseGenerator(np.array(data_a_z) + 2*g, gpsNoise)


# Setting up the filter

inital_control = np.zeros((3,1))
initial_state = np.zeros((5,1))
P = 500 * np.array([[1, 0, 0, 1, 0, 0],
                   [0, 1, 0, 0, 1, 0],
                   [0, 0, 1, 0, 0, 1],
                   [1, 0, 0, 1, 0, 0],
                   [0, 1, 0, 0, 1, 0],
                   [0, 0, 1, 0, 0, 1]])

kf = LinearKalmanFilter(initial_state, P, inital_control)

r_output_x = []
r_output_y = []
r_output_z = []
v_output_x = []
v_output_y = []
v_output_z = []

# Running the filter
for i in range(1, len(data_Time)):  
    dt = data_Time[i] - data_Time[i-1]  # Time difference between current and previous data
    measurement = np.array([data_r_meas_x[i], data_r_meas_y[i], data_r_meas_z[i]])
    control = np.array([data_a_meas_x[i], data_a_meas_y[i], data_a_meas_z[i]-g])

    # Perform Kalman Filter iteration
    kf = kf.iterate(dt, measurement, control)
    
    # Append updated state estimates to output arrays
    r_output_x.append(kf.x[0])
    r_output_y.append(kf.x[1])
    r_output_z.append(kf.x[2])
    v_output_x.append(kf.x[3])
    v_output_y.append(kf.x[4])
    v_output_z.append(kf.x[5])

# Create DataFrame from results
output = pd.DataFrame({
    'r_output_x': r_output_x,
    'r_output_y': r_output_y,
    'r_output_z': r_output_z,
    'v_output_x': v_output_x,
    'v_output_y': v_output_y,
    'v_output_z': v_output_z})
# Analyzing output

# Plot 1: Z Position vs Time
plt.figure()
plt.plot(data_Time, data_r_z, 'r-', label='Actual Z Position')
plt.plot(data_Time, data_r_meas_z, 'g.', label='Measured Z Position')
plt.plot(data_Time, r_output_z, 'b', label='Output Z Position')

plt.xlabel('Time (s)')
plt.ylabel('Z Position (m)')
plt.title('Z Position vs Time')

plt.legend()
plt.grid(True)
plt.show()

# Plot 2: x, y, z Positions vs Time (Subplots)
plt.figure()

# Subplot for x position

plt.subplot(3, 1, 1)
plt.plot(data_Time, data_r_x, 'r', label='Actual x')
plt.plot(data_Time, data_r_meas_x, 'g.', label='Measured X Position')
plt.plot(data_Time, r_output_x, 'b', label='Output x')

plt.xlabel('Time (s)')
plt.ylabel('x Position (m)')
plt.title('x Position vs Time')
plt.legend()
plt.grid(True)

# Subplot for y position
plt.subplot(3, 1, 2)
plt.plot(data_Time, data_r_y, 'r', label='Actual y')
plt.plot(data_Time, data_r_meas_y, 'g.', label='Measured Y Position')
plt.plot(data_Time, r_output_y, 'b', label='Output y')

plt.xlabel('Time (s)')
plt.ylabel('y Position (m)')
plt.title('y Position vs Time')
plt.legend()
plt.grid(True)

# Subplot for z position
plt.subplot(3, 1, 3)
plt.plot(data_Time, data_r_z, 'r', label='Actual z')
plt.plot(data_Time, data_r_meas_z, 'g.', label='Measured Z Position')
plt.plot(data_Time, r_output_z, 'b', label='Output z')

plt.xlabel('Time (s)')
plt.ylabel('z Position (m)')
plt.title('z Position vs Time')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()

# Plot 3: Z Velocity vs Time
plt.figure()
plt.plot(data_Time, data_v_z, 'r', label='Actual z Velocity')
plt.plot(data_Time, v_output_z, 'b', label='Output z Velocity')

plt.xlabel('Time (s)')
plt.ylabel('z Velocity (m/s)')
plt.title('z Velocity vs Time')
plt.legend()
plt.grid(True)
plt.show()

# Plot 4: x, y, z Velocities vs Time (Subplots)
plt.figure()

# Subplot for x velocity
plt.subplot(3, 1, 1)
plt.plot(data_Time, data_v_x, 'r', label='Actual x Velocity')
plt.plot(data_Time, output['v_output_x'], 'b', label='Output x Velocity')

plt.xlabel('Time (s)')
plt.ylabel('x Velocity (m/s)')
plt.title('x Velocity vs Time')

plt.legend()
plt.grid(True)

# Subplot for y velocity
plt.subplot(3, 1, 2)
plt.plot(data_Time, data_v_y, 'r', label='Actual y Velocity')
plt.plot(data_Time, v_output_y, 'b', label='Output y Velocity')

plt.xlabel('Time (s)')
plt.ylabel('y Velocity (m/s)')
plt.title('y Velocity vs Time')

plt.legend()
plt.grid(True)

# Subplot for z velocity
plt.subplot(3, 1, 3)
plt.plot(data_Time, data_v_z, 'r', label='Actual z Velocity')
plt.plot(data_Time, v_output_z, 'b', label='Output z Velocity')

plt.xlabel('Time (s)')
plt.ylabel('z Velocity (m/s)')
plt.title('z Velocity vs Time')

plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()
