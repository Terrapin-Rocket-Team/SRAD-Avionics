import subprocess
from datetime import datetime
import os
import serial
import time

# UART configuration
UART_PORT = "/dev/serial0"  # Default UART port on Raspberry Pi
BAUD_RATE = 9600

# Initialize UART communication
ser = serial.Serial(UART_PORT, BAUD_RATE, timeout=1)

# Video file path configuration
def get_file_paths():
    unix_time = int((datetime.now() - datetime(1970, 1, 1)).total_seconds())
    log_file_path = os.path.expanduser(f"~/video/log_{unix_time}.txt")
    video_file_path = os.path.expanduser(f"~/video/video_{unix_time}.mp4")
    return log_file_path, video_file_path

log_file_path, video_file_path = get_file_paths()
log_file = open(log_file_path, "w")

# Handy logging statements
def log_println(message):
    print(message)
    log_file.write(message + "\n")
    log_file.flush()

def start_video():
    cam = subprocess.Popen([
        "libcamera-vid", "-t", "300000", "--no-raw", "--codec", "yuv420", "--width", "1920", "--height", "1080", "--framerate", "30", "-o", "-"
    ], stdout=subprocess.PIPE, stderr=log_file)

    enc = subprocess.Popen([
        "ffmpeg", "-s", "1920x1080", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-i", "-",
        "-vcodec", "h264_v4l2m2m", "-r", "30", "-b:v", "10M", "-an", "-sn", "-dn", "-y", video_file_path
    ], stdin=cam.stdout, stderr=log_file)

    cam.wait()
    log_println("Video recording completed")

# Handle commands from FC
def process_command(command):
    global log_file, log_file_path, video_file_path

    if command == "START":
        log_println("START command received")
        ser.write(b"ACK_START\n")  # Acknowledge command
        log_file_path, video_file_path = get_file_paths()
        start_video()
        ser.write(b"DONE_START\n")  # Signal completion

    elif command == "STOP":
        log_println("STOP command received")
        ser.write(b"ACK_STOP\n")  # Acknowledge command
        log_println("Stop command does not interrupt a running recording.")
        ser.write(b"DONE_STOP\n")  # Signal completion

    else:
        log_println(f"Unknown command: {command}")
        ser.write(b"ERR_UNKNOWN_COMMAND\n")

def main():
    log_println("UART video handler started")

    try:
        while True:
            if ser.in_waiting > 0:
                command = ser.readline().decode("utf-8").strip()
                log_println(f"Command received: {command}")
                process_command(command)

            time.sleep(0.1)  # Prevent busy looping

    except KeyboardInterrupt:
        log_println("Exiting program")

    finally:
        ser.close()
        log_file.close()

if __name__ == "__main__":
    main()
