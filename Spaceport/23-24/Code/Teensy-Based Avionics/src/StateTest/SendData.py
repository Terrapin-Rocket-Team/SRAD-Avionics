import serial
import time
from colorama import Fore

serial_port = 'COM3'  # Use the correct port for the Arduino
baud_rate = 9600  # Match the baud rate to the Arduino's
data_file = 'fake_data.csv'
output = 'test_results.csv'
good = True  # Flag to track if we should send the next line

# Open serial connection to Arduino
ser = serial.Serial(serial_port, baud_rate)
time.sleep(2)  # Wait for the connection to establish

if ser.is_open:
    with open(data_file, 'r') as file, open(output, 'w', newline='') as out_csv:
        out_csv.write("timeAbsolute, timeSinceLaunch,stage,timeSincePreviousStage,AX,AY,AZ,VX,VY,VZ,PX,PY,PZ,OX,OY,OZ,OW,APOGEE\n")
        waitForResponse = False  # Flag to check if we are waiting for specific response
        for line in file:
            line = line.strip()
            if line and not line.startswith("time"):
                while True:  # Keep checking until a correct response is received
                    if good and not waitForResponse:
                        print(f"{Fore.CYAN}" + 'S-' + line + Fore.RESET)
                        ser.write(line.encode() + b'\n')
                        waitForResponse = True  # Now wait for the response
                    if ser.in_waiting > 0:
                        response = ser.readline().decode().strip()
                        print(f"{Fore.LIGHTGREEN_EX}" + 'R-' + response + Fore.RESET)
                        if response.startswith("[][]"):
                            out_csv.write(response.replace("|", ",") + '\n')
                            waitForResponse = False  # Got the response, can send the next line
                            break  # Exit the while loop to proceed with next line
                    time.sleep(0.1)  # Slight delay to prevent CPU overload

ser.close()
