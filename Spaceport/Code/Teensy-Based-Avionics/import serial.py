import serial
import time
import os

COM_PORT = 'COM31'         # Change this to your port name, e.g., '/dev/ttyUSB0' on Linux
BAUD_RATE = 1000000         # Change as needed
OUTPUT_FILE = 'output.ivf'  # File to write data to
TIMEOUT_AFTER_DATA = 5   # seconds to wait after last data before closing

def listen_and_write():
    with serial.Serial(COM_PORT, BAUD_RATE, timeout=0.2) as ser, open(OUTPUT_FILE, 'wb') as f:
        print(f"Listening on {COM_PORT} at {BAUD_RATE} baud...")
        last_data_time = None
        
        while True:
            data = ser.read(1024*7)  # read up to 1024 bytes
            if data:
                f.write(data)
                f.flush()
                last_data_time = time.time()
                print(f"Received {len(data)} bytes...")
            else:
                # If we've seen data and timeout expired, stop listening
                if last_data_time and (time.time() - last_data_time) > TIMEOUT_AFTER_DATA:
                    print("Timeout expired after last data received. Stopping.")
                    print(f"File size: {os.path.getsize(OUTPUT_FILE)}")
                    break

if __name__ == '__main__':
    listen_and_write()
