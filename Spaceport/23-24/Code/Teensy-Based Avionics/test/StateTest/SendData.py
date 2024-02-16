import serial
import time
import csv


serial_port = 'COM3'  # Use the correct port for the Arduino
baud_rate = 9600  # Match the baud rate to the Arduino's
data_file = 'fake_data.csv'
output = 'test_results.csv'

# Open serial connection to Arduino
ser = serial.Serial(serial_port, baud_rate,timeout=1)
time.sleep(2)  # Wait for the connection to establish
if(ser.is_open):
    with open(data_file, 'r') as file, open(output, 'w', newline='') as out_csv:
        csv_writer = csv.writer(out_csv)
        for line in file:
            line = line.strip()
            if line:
                ser.write(line.encode() + b'\n')
                time.sleep(0.1)

                # Wait for a response and read it
                while ser.in_waiting > 0:
                    response = ser.readline().decode().strip()
                    csv_writer.writerow([response])

                    
ser.close()
