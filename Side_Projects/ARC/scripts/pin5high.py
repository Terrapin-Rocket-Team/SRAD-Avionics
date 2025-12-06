import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BOARD)
GPIO.setup(5, GPIO.OUT)

GPIO.output(5, GPIO.HIGH)
time.sleep(0.1)
GPIO.output(5, GPIO.LOW)

GPIO.cleanup()
