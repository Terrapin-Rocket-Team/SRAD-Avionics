#DataGenerator
import rocket
import csv
import pandas as pd

dataFileName = "mock_data"
def DataGenerator(dataFileName,loopFrequency,rocket):
    density = 1.225 #kg/m^3

    #initialization values
    i = 1
    dt = 1/loopFrequency

    a_x = []
    a_y = []
    a_z = []

    a_x[1] = 0; # acceleration in x (m/s^2)
    a_y[1] = 0; # acceleration in y (m/s^2)
    a_z[1] = -9.8; # acceleration in z (m/s^2)

    v_x = []
    v_y = []
    v_z = []

    v_x[1] = 0; # (m/s)
    v_y[1] = 0; # (m/s)
    v_z[1] = 0; # (m/s)

    r_x = []
    r_y = []
    r_z = []

    r_x[1] = 0; # start position in x (m)
    r_y[1] = 0; # start position in y (m)
    r_z[1] = 0.1; # start position in z (m)

    t= []

    t[1] = 0; # start time (s)
    # Run simple propagation
    while r_z[1] > 0:
        i = i + 1;
    t[1] = t[i-1] + dt;
    
    # Update accelerations (x and y remain 0)
    a_x[i] = 0;
    a_y[i] = 0;
    if v_z[i-1] < 0:
        aoa = 1; # angle of attack flag to determine drag direction
    else:
        aoa = -1;
    
    if t[i] < rocket.burnTime:
        a_z[i] = rocket.motorAccel + aoa*.5*density*rocket.dragCoef*rocket.crossSectionalArea*v_z(i-1) - 9.8;
    else:
        a_z[i] = aoa*.5*density*rocket.dragCoef*rocket.crossSectionalArea*v_z(i-1) - 9.8;
    
    
    # Update velocities using 1D kinematics in each direction
    v_x[i] = v_x[i-1] + dt * a_x[i];
    v_y[i] = v_y[i-1] + dt * a_y[i];
    v_z[i] = v_z[i-1] + dt * a_z[i];
    
    # Update positions using 1D kinematics in each direction
    r_x[i] = r_x[i-1] + v_x[i] * dt + 0.5 * a_x[i] * dt^2;
    r_y[i] = r_y[i-1] + v_y[i] * dt + 0.5 * a_y[i] * dt^2;
    r_z[i] = r_z[i-1] + v_z[i] * dt + 0.5 * a_z[i] * dt^2;        

    with open('{dataFileName}.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerows(t)
        

DataGenerator(dataFileName,2,rocket)