import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Replace with your serial port and baud rate
SERIAL_PORT = "COM4"  # Update this to your serial port (e.g., "/dev/ttyUSB0" on Linux/Mac)
BAUD_RATE = 9600

# Open the serial port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Data storage for plotting
time_data = []
z_data = []
y_data = []
x_data = []

# Plot parameters
MAX_POINTS = 100  # Maximum number of points to show
time_counter = 0  # To simulate time axis

# Function to read data from the serial port
def read_serial_data():
    global time_counter
    line = ser.readline().decode('utf-8').strip()  # Read a line and decode
    try:
        z, y, x = map(float, line.split('|'))  # Split and convert to float
        return time_counter, z, y, x
    except ValueError:
        return None  # Handle any malformed data gracefully

# Update function for the animation
def update(frame):
    global time_counter
    data = read_serial_data()
    if data is not None:
        t, z, y, x = data
        time_counter += 1
        if(time_counter % 2 == 0):
        # Append data
            time_data.append(t)
            z_data.append(z)
            y_data.append(y)
            x_data.append(x)
        
        # Limit the data to the MAX_POINTS
            if len(time_data) > MAX_POINTS:
                time_data.pop(0)
                z_data.pop(0)
                y_data.pop(0)
                x_data.pop(0)
        
        # Clear and re-plot data
            ax.clear()
            ax.plot(time_data, z_data, label='Z (Yaw)')
            ax.plot(time_data, y_data, label='Y (Pitch)')
            ax.plot(time_data, x_data, label='X (Roll)')
            ax.legend(loc='upper left')
            ax.set_title("Live Euler Angles")
            ax.set_xlabel("Time (frames)")
            ax.set_ylabel("Angle (degrees)")
            ax.grid(True)

            ax.set_ylim(-180, 180)

# Create the plot
fig, ax = plt.subplots(figsize=(10, 6))
ani = FuncAnimation(fig, update, interval=50)  # Update every 100ms

# Show the plot
plt.show()

# Close the serial port when done
ser.close()
