import RPi.GPIO as GPIO
import subprocess
import time

# Setup
CMD_PIN= 15  # GPIO pin number
RESP_PIN = 16 # response pin
GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(RESP_PIN, GPIO.OUT)

recording = False
process = None

def start_recording():
    global recording, process
    if not recording:
        print("Starting recording...")
        process = subprocess.Popen([
            "libcamera-vid",
            #"-o", "video.mp4",
            "-t", "0",
            "-n",
            "--width", "1080",
            "--height", "720",
            "--hdr", "auto",
            "--framerate", "15",
            "--listen",
            "-o", "tcp://0.0.0.0:5000"
        ])
        recording = True
        GPIO.output(RESP_PIN, GPIO.HIGH)

def stop_recording():
    global recording, process
    if recording and process:
        print("Stopping recording...")
        process.terminate()  # Gracefully terminate the process
        process.wait()       # Wait for the process to clean up
        process = None
        recording = False
        GPIO.output(RESP_PIN, GPIO.LOW)

try:
    # Start recording automatically on boot
    start_recording()

    while True:
        if GPIO.input(CMD_PIN) == GPIO.HIGH:
            stop_recording()
        else:
            start_recording()
        time.sleep(0.1)  # Debounce and CPU usage control

except KeyboardInterrupt:
    print("Exiting...")
finally:
    GPIO.output(RESP_PIN, GPIO.LOW)
    GPIO.cleanup()