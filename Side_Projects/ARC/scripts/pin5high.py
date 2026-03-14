import RPi.GPIO as GPIO
import time

PIN = 7

GPIO.setmode(GPIO.BOARD)
GPIO.setup(PIN, GPIO.OUT)

GPIO.output(PIN, GPIO.HIGH)
time.sleep(0.1)
GPIO.output(PIN, GPIO.LOW)

GPIO.cleanup()
