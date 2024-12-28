from rocket_module import *
from KalmanFilter import *
from GaussianNoiseGenerator import *
from DataGenerator import *
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import time

from enum import Enum


class DataType(Enum):
    Mock = 1
    OpenRocket = 2
    Flight = 3

run_kf = True

# Setting sensor gaussian noise
accelNoise = 0.1
baroNoise = 0.5
gpsNoise = 1
g = 9.81

MOCK = DataType.Mock.value
OPENROCKET = DataType.OpenRocket.value
FLIGHTDATA = DataType.Flight.value

dataType = MOCK

if dataType == MOCK:

    datafile_name = f"mock_{time.time()}"       # change after underscore to be a non-number to commit file to github

    sim_rocket = rocket_module.Rocket(250,5,0.8,.07296) # (MotorAccel, BurnTime, DragCoef, CrossSectionalArea)


    #  First create mock data
    DataGenerator(datafile_name, loopFrequency, sim_rocket)

    # Read and assign mock data
    df = pd.read_csv(datafile_name + ".csv")

    data_Time = df["t"].to_numpy()

    data_r_x = df["r_x"].to_numpy()
    data_r_y = df["r_y"].to_numpy()
    data_r_z = df["r_z"].to_numpy()

    data_v_x = df["v_x"].to_numpy()
    data_v_y = df["v_y"].to_numpy()
    data_v_z = df["v_z"].to_numpy()

    data_a_x = df["a_x"].to_numpy()
    data_a_y = df["a_y"].to_numpy()
    data_a_z = df["a_z"].to_numpy()


if dataType == OPENROCKET:
    df = pd.read_csv("openrocket_2024_30k.csv")
    data_Time = df["# Time (s)"].to_numpy()

    data_r_x = df["Position East of launch (m)"].to_numpy()
    data_r_y = df["Positions North of launch (m)"].to_numpy()
    data_r_z = df["Altitude (m)"].to_numpy()

    # Diff poition to get velocity
    delta_T = np.diff(data_Time)
    delta_r_x = np.diff(data_r_x)
    delta_r_y = np.diff(data_r_y)

    data_v_x = [delta_r_x / delta_T]
    data_v_y = [delta_r_y / delta_T]
    data_v_z = df["Vertical velocity (m/s)"].to_numpy()

    # Diff velocity to get acceleration
    delta_v_x = np.diff(data_v_x)
    delta_v_y = np.diff(data_v_y)

    data_a_x = [delta_v_x / delta_T]
    data_a_y = [delta_v_y / delta_T]
    data_a_z = df["Vertical Acceleration"].to_numpy()

# Read in Flight Data File
if dataType == FLIGHTDATA:
    df = pd.read_csv("TADPOL_April_NY_post_processed2.csv")
    data_Time = df["Time (ms)"].to_numpy()

    data_r_x = df["PosX (m)"].to_numpy()
    data_r_y = df["PosY (m)"].to_numpy()
    data_r_z = df["PosZ (m)"].to_numpy()

    data_v_x = df["VeloX (m/s)"].to_numpy()
    data_v_y = df["VeloY (m/s)"].to_numpy()
    data_v_z = df["VeloZ (m/s)"].to_numpy()

    data_a_x = df["AccelX (m/s^2)"].to_numpy()
    data_a_y = df["AccelY (m/s^2)"].to_numpy()
    data_a_z = df["AccelZ (m/s^2)"].to_numpy()

# Add gaussian noise to generate measurement data
if dataType != FLIGHTDATA:
    data_r_meas_x = GaussianNoiseGenerator(data_r_x, gpsNoise)
    data_r_meas_y = GaussianNoiseGenerator(data_r_y, gpsNoise)
    data_r_meas_z = GaussianNoiseGenerator(data_r_z, gpsNoise)
    data_a_meas_x = GaussianNoiseGenerator(data_a_x, gpsNoise)
    data_a_meas_y = GaussianNoiseGenerator(data_a_y, gpsNoise)
    data_a_meas_z = GaussianNoiseGenerator(np.array(data_a_z) + 2 * g, gpsNoise)


# Setting up the filter
    
if run_kf:
    
    inital_control = np.zeros((3, 1))
    initial_state = np.zeros((6, 1))
    P = 500 * np.array([
            [1, 0, 0, 1, 0, 0],
            [0, 1, 0, 0, 1, 0],
            [0, 0, 1, 0, 0, 1],
            [1, 0, 0, 1, 0, 0],
            [0, 1, 0, 0, 1, 0],
            [0, 0, 1, 0, 0, 1],
        ])

    kf = LinearKalmanFilter(initial_state, P, inital_control)

    r_output_x = []
    r_output_y = []
    r_output_z = []
    v_output_x = []
    v_output_y = []
    v_output_z = []

    # Running the filter
    for i in range(1, len(data_Time)):
        dt = (data_Time[i] - data_Time[i - 1])  # Time difference between current and previous data
        measurement = np.array([
            [data_r_meas_x[i]],
            [data_r_meas_y[i]],
            [data_r_meas_z[i]]
            ])
        control = np.array([
            [data_a_meas_x[i]],
            [data_a_meas_y[i]],
            [data_a_meas_z[i] - g]
            ])
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
    output = pd.DataFrame(
        {
            "r_output_x": r_output_x,
            "r_output_y": r_output_y,
            "r_output_z": r_output_z,
            "v_output_x": v_output_x,
            "v_output_y": v_output_y,
            "v_output_z": v_output_z,
        })
    r_output_x = output["r_output_x"].to_numpy()
    r_output_y = output["r_output_y"].to_numpy()
    r_output_z = output["r_output_z"].to_numpy()
    v_output_x = output["v_output_x"].to_numpy()
    v_output_y = output["v_output_y"].to_numpy()
    v_output_z = output["v_output_z"].to_numpy()   
    


#       Plot 1: Z Position vs Time
# !!! [:-1] is so the sizes between data_Tiem and r_output_z match for the time being

plt.figure()
plt.plot(data_Time[:-1], data_r_z[:-1], "r-", label="Actual Z Position") 
plt.plot(data_Time[:-1], data_r_meas_z[:-1], "g.", label="Measured Z Position")
if run_kf:
    plt.plot(data_Time[:-1], r_output_z, "b", label="Output Z Position")

plt.xlabel("Time (s)")
plt.ylabel("Z Position (m)")
plt.title("Z Position vs Time")

plt.legend()
plt.grid(True)
plt.show()

# Plot 2: x, y, z Positions vs Time (Subplots)
plt.figure()

# Subplot for x position

plt.subplot(3, 1, 1)
plt.plot(data_Time[:-1], data_r_x[:-1], "r", label="Actual x")
plt.plot(data_Time[:-1], data_r_meas_x[:-1], "g.", label="Measured X Position")
if run_kf:
    plt.plot(data_Time[:-1], r_output_x, "b", label="Output x")

plt.xlabel("Time (s)")
plt.ylabel("x Position (m)")
plt.title("x Position vs Time")
plt.legend()
plt.grid(True)

# Subplot for y position
plt.subplot(3, 1, 2)
plt.plot(data_Time[:-1], data_r_y[:-1], "r", label="Actual y")
plt.plot(data_Time[:-1], data_r_meas_y[:-1], "g.", label="Measured Y Position")
if run_kf:
    plt.plot(data_Time[:-1], r_output_y, "b", label="Output y")

plt.xlabel("Time (s)")
plt.ylabel("y Position (m)")
plt.title("y Position vs Time")
plt.legend()
plt.grid(True)

# Subplot for z position
plt.subplot(3, 1, 3)
plt.plot(data_Time[:-1], data_r_z[:-1], "r", label="Actual z")
plt.plot(data_Time[:-1], data_r_meas_z[:-1], "g.", label="Measured Z Position")
if run_kf:
    plt.plot(data_Time[:-1], r_output_z, "b", label="Output z")

plt.xlabel("Time (s)")
plt.ylabel("z Position (m)")
plt.title("z Position vs Time")
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()

# Plot 3: Z Velocity vs Time
plt.figure()
plt.plot(data_Time[:-1], data_v_z[:-1], "r", label="Actual z Velocity")
if run_kf:
    plt.plot(data_Time[:-1], v_output_z, "b", label="Output z Velocity")

plt.xlabel("Time (s)")
plt.ylabel("z Velocity (m/s)")
plt.title("z Velocity vs Time")
plt.legend()
plt.grid(True)
plt.show()

# Plot 4: x, y, z Velocities vs Time (Subplots)
plt.figure()

# Subplot for x velocity
plt.subplot(3, 1, 1)
plt.plot(data_Time[:-1], data_v_x[:-1], "r", label="Actual x Velocity")
if run_kf:
    plt.plot(data_Time[:-1], v_output_x, "b", label="Output x Velocity")

plt.xlabel("Time (s)")
plt.ylabel("x Velocity (m/s)")
plt.title("x Velocity vs Time")

plt.legend()
plt.grid(True)

# Subplot for y velocity
plt.subplot(3, 1, 2)
plt.plot(data_Time[:-1], data_v_y[:-1], "r", label="Actual y Velocity")
if run_kf:
    plt.plot(data_Time[:-1], v_output_y, "b", label="Output y Velocity")

plt.xlabel("Time (s)")
plt.ylabel("y Velocity (m/s)")
plt.title("y Velocity vs Time")

plt.legend()
plt.grid(True)

# Subplot for z velocity
plt.subplot(3, 1, 3)
plt.plot(data_Time[:-1], data_v_z[:-1], "r", label="Actual z Velocity")
if run_kf:
    plt.plot(data_Time[:-1], v_output_z, "b", label="Output z Velocity")

plt.xlabel("Time (s)")
plt.ylabel("z Velocity (m/s)")
plt.title("z Velocity vs Time")

plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()
