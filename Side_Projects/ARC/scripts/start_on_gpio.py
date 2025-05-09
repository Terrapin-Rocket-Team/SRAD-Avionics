import RPi.GPIO as GPIO
import subprocess

pin = 5

def start_video():
    print("Started recording video")
    subprocess.run(["bash", "/home/lvpi1/lv-github/tests/localsave1080pv2.sh", "&"])
    GPIO.remove_event_detect(pin)

def main():
    print("Waiting for signal on pin " + str(pin))
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(pin, GPIO.IN)
    GPIO.wait_for_edge(pin, GPIO.RISING)
    start_video()

main()
