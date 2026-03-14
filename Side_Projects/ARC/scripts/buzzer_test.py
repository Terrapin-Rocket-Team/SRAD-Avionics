from gpiozero import PWMLED
from time import sleep
from buzzer import buzzer

import subprocess
camera_check = subprocess.Popen("rpicam-vid --list-cameras".split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

try:
    out, err = camera_check.communicate(timeout=10)
    if (len(out) > 0):
        if ("Available cameras" in str(out)):
            buzzer.blink(1,1,buzzer.INFO_FREQ)
            print("OUT: ")
            print(out.decode("ascii"))
        else:
            buzzer.blink(0.2,2,buzzer.ERR_FREQ)

    if (len(err) > 0):
        print("ERR: ")
        print(str(err))
        buzzer.blink(0.2,3,buzzer.ERR_FREQ)
except subprocess.TimeoutExpired:
    camera_check.kill()
    buzzer.blink(0.2,4,buzzer.ERR_FREQ)