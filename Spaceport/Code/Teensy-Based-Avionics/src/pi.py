import RPi.GPIO as GPIO
import subprocess
import time
import signal
from datetime import datetime
# GPIO Configuration
CMD_PIN = 6    # Input pin (from Teensy)
GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_PIN, GPIO.IN)

# Process control
recording = False
ffmpeg_process = None

unixTime = int((datetime.now() - datetime(1970, 1, 1)).total_seconds())

def start_recording():
    global ffmpeg_process
    command = [
        'raspicam-vid',
        '-t', '0', # Input resolution
        '--width', '1920',        # Input framerate
        '--height', '1080',       # Camera device
        '-framerate', '30',        # AV1 encoding via rav1e
        '-o', f'output_{str(unixTime)}.ivf',             # Speed preset (0-10, 6=real-time)
    ]
    ffmpeg_process = subprocess.Popen(command, 
                                    stdin=subprocess.PIPE,
                                    preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_IGN))

def stop_recording():
    global ffmpeg_process
    if ffmpeg_process:
        ffmpeg_process.stdin.write(b'q')
        ffmpeg_process.stdin.close()
        ffmpeg_process.wait()
        ffmpeg_process = None

def cmd_callback(channel):
    global recording
    cmd_state = GPIO.input(CMD_PIN)
    
    if cmd_state == GPIO.LOW and not recording:
        # Start recording
        start_recording()
        recording = True
        print("Recording started")
        
    elif cmd_state == GPIO.HIGH and recording:
        # Stop recording
        stop_recording()
        recording = False
        print("Recording stopped")

# Add interrupt detection for both edges
GPIO.add_event_detect(CMD_PIN, GPIO.BOTH, 
                     callback=cmd_callback, 
                     bouncetime=50)

try:
    print("Waiting for commands...")
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    stop_recording()
    GPIO.cleanup()
    print("\nExiting...")