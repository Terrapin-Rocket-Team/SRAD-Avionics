from gpiozero import PWMLED
from time import sleep

BUZZER_PIN = 26

ERR_FREQ = 3100
INFO_FREQ = 2700

b = PWMLED(BUZZER_PIN, frequency=INFO_FREQ)

def on(frequency=INFO_FREQ):
    b.frequency = frequency
    b.value = 0.5

def off():
    b.value = 0

def blink(interval, times, frequency=INFO_FREQ):
    for i in range(times):
        on(frequency)
        sleep(interval)
        off()
        sleep(interval)

buzzer = type('', (), {})()
buzzer.on = on
buzzer.off = off
buzzer.blink = blink
buzzer.INFO_FREQ = INFO_FREQ
buzzer.ERR_FREQ = ERR_FREQ