import rocket_module
import csv
import pandas as pd

dataFileName = "mock_data"
loopFrequency = 50
rocket_instance = rocket_module.Rocket(125,2.5,0.5,.07296) # (MotorAccel, BurnTime, DragCoef, CrossSectionalArea)

def DataGenerator(dataFileName, loopFrequency, rocket):
    density = 1.225  # kg/m^3

    # initialization values
    i = 0
    dt = 1 / loopFrequency

    # Initialize acceleration, velocity, and position lists
    a_x = [0]  # acceleration in x (m/s^2)
    a_y = [0]  # acceleration in y (m/s^2)
    a_z = [-9.8]  # acceleration in z (m/s^2)

    v_x = [0]  # (m/s)
    v_y = [0]  # (m/s)
    v_z = [0]  # (m/s)

    r_x = [0]  # start position in x (m)
    r_y = [0]  # start position in y (m)
    r_z = [0.1]  # start position in z (m)

    t = [0]  # start time (s)

    # Run simple propagation
    while r_z[i] > 0:
        i += 1
        t.append(t[i-1] + dt)  # Update time

        # Update accelerations (x and y remain 0)
        a_x.append(0)
        a_y.append(0)

        # Determine the angle of attack (aoa) based on velocity
        if v_z[i-1] < 0:
            aoa = 1  # angle of attack flag to determine drag direction
        else:
            aoa = -1

        # Update vertical acceleration based on rocket motor status
        if t[i] < rocket.burnTime:
            a_z.append(rocket.motorAccel + aoa* 0.5*density*rocket.dragCoef*rocket.crossSectionalArea*v_z[i-1] - 9.8)
        else:
            a_z.append(aoa*0.5*density*rocket.dragCoef*rocket.crossSectionalArea*v_z[i-1] - 9.8)

        # Update velocities using 1D kinematics in each direction
        v_x.append(v_x[i-1] + (dt * a_x[i]))
        v_y.append(v_y[i-1] + (dt * a_y[i]))
        v_z.append(v_z[i-1] + (dt * a_z[i]))

        # Update positions using 1D kinematics in each direction
        r_x.append(r_x[i-1] + v_x[i] * dt + 0.5 * a_x[i] * dt**2)
        r_y.append(r_y[i-1] + v_y[i] * dt + 0.5 * a_y[i] * dt**2)
        r_z.append(r_z[i-1] + v_z[i] * dt + 0.5 * a_z[i] * dt**2)



    # Write the data to a CSV file
    with open(f'{dataFileName}.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["t", "r_x", "r_y", "r_z", "v_x", "v_y", "v_z", "a_x", "a_y", "a_z"])
        for idx in range(len(t)):
            writer.writerow([t[idx], r_x[idx], r_y[idx], r_z[idx], v_x[idx], v_y[idx], v_z[idx], a_x[idx], a_y[idx], a_z[idx]])

