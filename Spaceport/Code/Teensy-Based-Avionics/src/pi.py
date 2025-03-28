import RPi.GPIO as GPIO
import subprocess
import time
import signal

# GPIO Configuration
CMD_PIN = 6    # Input pin (from Teensy)
RESP_PIN = 5   # Output pin (to Teensy)
GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(RESP_PIN, GPIO.OUT, initial=GPIO.HIGH)

# Process control
recording = False
ffmpeg_process = None

def start_recording():
    global ffmpeg_process
    command = [
        'ffmpeg',
        '-video_size', '1280x720', # Input resolution
        '-framerate', '30',        # Input framerate
        '-i', '/dev/video0',       # Camera device
        '-c:v', 'librav1e',        # AV1 encoding via rav1e
        '-speed', '6',             # Speed preset (0-10, 6=real-time)
        '-threads', '4',           # Number of encoding threads
        '-qp', '40',               # Quality parameter (adjust as needed)
        '-y',                      # Overwrite output
        'output.ivf'               # Output path (IVF container)
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
        GPIO.output(RESP_PIN, GPIO.LOW)
        print("Recording started")
        
    elif cmd_state == GPIO.HIGH and recording:
        # Stop recording
        stop_recording()
        recording = False
        GPIO.output(RESP_PIN, GPIO.HIGH)
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