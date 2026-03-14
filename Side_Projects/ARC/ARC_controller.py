#!/bin/python -u

import RPi.GPIO as GPIO
from datetime import datetime
import subprocess
import time
import os
import re
import atexit
import signal
import select
from py_libs.buzzer import buzzer

# GPIO Configuration
CMD_PIN = 6    # Input pin (from Teensy)
INTERFACE_PIN = 13 # Input pin (external source)
WIFI_RST_PIN = 19 # Input pin (external source)
LED_PIN = 4
GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(INTERFACE_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(WIFI_RST_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(LED_PIN, GPIO.OUT)
GPIO.output(LED_PIN, GPIO.LOW)
unixTime = int((datetime.now() - datetime(1970, 1, 1)).total_seconds())

delayTime = 900

logFile = open(os.path.expanduser("~/ARC_log/" + str(unixTime) + "_log.txt"), "w")

# handy logging functions
def logPrintln(s):
    s = "[" + str(datetime.now().time()) + "] " + s
    print(s)
    logFile.write(s + "\n")
    logFile.flush()


logPrintln("ARC Controller startup complete")

# process control
video = False
live = False
interface = False
exited = False

# subprocesses
rpicam_process = None
av1_process = None
transmit_process = None
interface_process = None
interface_select = None
stream_process = None

# command strings
command_rpicam = "rpicam-vid -t 0 --buffer-count 6 --width 1280 --height 720 --framerate 60 --nopreview --no-raw --flush -o "
command_av1 = "aomenc -w 1280 -h 720 --profile=0 --kf-max-dist=300 --end-usage=cbr --min-q=1 --max-q=38 " \
"--undershoot-pct=100 --overshoot-pct=25 --buf-sz=20 --buf-initial-sz=10 --buf-optimal-sz=10 --max-intra-rate=600 " \
"--passes=1 --lag-in-frames=0 --error-resilient=0 --tile-columns=1 --tile-rows=3 --aq-mode=2 --enable-obmc=0 " \
"--enable-global-motion=1 --enable-warped-motion=0 --deltaq-mode=0 --enable-tpl-model=1 --mode-cost-upd-freq=2 " \
"--coeff-cost-upd-freq=2 --enable-ref-frame-mvs=0 --mv-cost-upd-freq=3 --enable-order-hint=0 --drop-frame=0 " \
"--resize-mode=3 --cpu-used=5 --rt --usage=1 --threads=3 --obu --target-bitrate=450 --fps=30/1 - -o "
command_transmit = "teensy-interface"
command_interface = "node ARC_utilities/app.js"
# assumes username is the same as hostname, WILL NOT WORK OTHERWISE
command_stream = "ffmpeg -f rawvideo -s 1280x720 -r 30 -i - -listen 1 -preset ultrafast -tune zerolatency -f mp4 -pix_fmt yuv420p -x264-params keyint=1 -g 1 -movflags +faststart+frag_keyframe+empty_moov -r 30 http://" + str(os.environ["LOGNAME"]) + ".local:7999"

# check cameras
camera_check = subprocess.Popen("rpicam-vid --list-cameras".split(" "), stdout=subprocess.PIPE, stderr=subprocess.PIPE)

try:
    out, err = camera_check.communicate(timeout=10)
    if (len(out) > 0):
        print(out.decode("ascii"))
        if ("Available cameras" in str(out)):
            buzzer.blink(1,1,buzzer.INFO_FREQ)
            print("OUT: ")
        else:
            buzzer.blink(0.2,2,buzzer.ERR_FREQ)

    if (len(err) > 0):
        print("ERR: ")
        print(err.decode("ascii"))
        buzzer.blink(0.2,3,buzzer.ERR_FREQ)
except subprocess.TimeoutExpired:
    camera_check.kill()
    buzzer.blink(0.2,4,buzzer.ERR_FREQ)

def led_blink(times, interval):
    for i in range(times):
        GPIO.output(LED_PIN, GPIO.HIGH)
        time.sleep(interval)
        GPIO.output(LED_PIN, GPIO.LOW)
        time.sleep(interval)

def led_on():
    GPIO.output(LED_PIN, GPIO.HIGH)

def led_off():
    GPIO.output(LED_PIN, GPIO.LOW)

led_off()

def start_recording():
    global rpicam_process
    global av1_process
    global command_rpicam
    global command_av1

    rpicam_process = subprocess.Popen((command_rpicam + os.path.expanduser("~/ARC_video/" + str(unixTime) + "_video.mp4")).split(" "))
    #av1_process = subprocess.Popen((command_av1 + os.path.expanduser("~/ARC_video/" + str(unixTime) + "_video.av1")).split(" "),
    #                                stdin=rpicam_process.stdout)

    led_on()
    buzzer.blink(0.5,2,buzzer.INFO_FREQ)

def start_transmitting():
    global rpicam_process
    global av1_process
    global transmit_process
    global command_rpicam
    global command_av1
    global command_transmit

    rpicam_process = subprocess.Popen(command_rpicam.split(" "),
                                    stdout=subprocess.PIPE)
    av1_process = subprocess.Popen((command_av1 + "-").split(" "),
                                    stdin=rpicam_process.stdout, stdout=subprocess.PIPE)
    transmit_process = subprocess.Popen(command_transmit.split(" "), stdin=av1_process.stdout)

    led_on()
    buzzer.blink(0.5,2,buzzer.INFO_FREQ)

def start_streaming():
    global rpicam_process
    global stream_process
    global command_rpicam
    global command_stream

    rpicam_process = subprocess.Popen(command_rpicam.split(" "),
                                    stdout=subprocess.PIPE)
    stream_process = subprocess.Popen(command_stream.split(" "), stdin=rpicam_process.stdout)

    led_on()
    buzzer.blink(0.5,2,buzzer.INFO_FREQ)

def stop_video():
    global rpicam_process
    global av1_process
    global transmit_process
    global stream_process

    if rpicam_process:
        rpicam_process.kill()
        rpicam_process.wait()
        rpicam_process = None
    if av1_process:
        av1_process.kill()
        av1_process.wait()
        av1_process = None
    if transmit_process:
        transmit_process.kill()
        transmit_process.wait()
        transmit_process = None
    if stream_process:
        stream_process.kill()
        stream_process.wait()
        stream_process = None

    led_off()
    buzzer.blink(1,2,buzzer.INFO_FREQ)

def video_callback(channel):
    global video
    cmd_state = GPIO.input(CMD_PIN)

    if cmd_state == GPIO.HIGH and not video:
        # Start recording
        if not live:
            start_recording()
        if live:
            start_transmitting()
        video = True
        # GPIO.output(RESP_PIN, GPIO.LOW)
        logPrintln("Recording started")

    elif cmd_state == GPIO.LOW and video:
        # Stop recording
        stop_video()
        video = False
        # GPIO.output(RESP_PIN, GPIO.HIGH)
        logPrintln("Recording stopped")

def start_interface():
    global interface_process
    global command_interface
    global interface_select

    buzzer.blink(0.2,1)
    interface_process = subprocess.Popen(command_interface.split(" "), stdout=subprocess.PIPE)
    interface_select = select.poll()
    interface_select.register(interface_process.stdout, select.POLLIN)

# string to indicate a command from the interface
cmd_trigger = "PYCMD"
# separator between trigger and command
cmd_sep = r"\|"
cmd_regex = cmd_trigger + cmd_sep + r".+"
#print(cmd_regex)
# list of possible commands and their corresponding callbacks
cmd_list = ["start recording", "start transmitting", "start streaming", "stop video"]
cmd_callbacks = [start_recording, start_transmitting, start_streaming, stop_video]

def process_interface():
    global interface_process
    global cmd_trigger
    global cmd_sep
    global cmd_list
    global cmd_callbacks
    global interface_select

    # get a line
    if interface_select.poll(0):
        line = interface_process.stdout.readline()
        if line:
            # check for cmd_trigger and sep
            m = re.search(cmd_regex, line.decode('UTF-8'))
            if m:
                # if match get command part (after sep character)
                cmd = re.split(cmd_sep, m.group())[1].strip()
                # find correct callback
                for i in range(len(cmd_list)):
                    if cmd_list[i] == cmd:
                        logPrintln("Running command " + str(cmd))
                        # all functions return None except the "stop interface callback"
                        if cmd_callbacks[i]():
                            return False
    return True

def stop_interface():
    global interface_process
    buzzer.blink(0.2,1)
    if interface_process:
        interface_process.kill()
        interface_process.wait()
        interface_process = None

def interface_callback(channel):
    global interface
    cmd_state = GPIO.input(INTERFACE_PIN)
    if cmd_state == GPIO.HIGH and not interface:
        time.sleep(0.5)
        start_interface()
        interface = True
        logPrintln("Started ARC interface")
        while process_interface() and GPIO.input(INTERFACE_PIN) != GPIO.HIGH:
            time.sleep(0.1)
        stop_interface()
        interface = False
        logPrintln("Stopped ARC interface")

def wifi_callback(channel):
    cmd_state = GPIO.input(WIFI_RST_PIN)
    if cmd_state == GPIO.HIGH:
        buzzer.blink(0.2,2)
        os.system("sudo nmcli device down wlan0 && sleep 5 && sudo nmcli device up wlan0")

# Add interrupt detection for both edges
GPIO.add_event_detect(CMD_PIN, GPIO.BOTH,
                     callback=video_callback,
                     bouncetime=50)

# Add interrupt detection for both edges
GPIO.add_event_detect(INTERFACE_PIN, GPIO.RISING,
                     callback=interface_callback,
                     bouncetime=50)

GPIO.add_event_detect(WIFI_RST_PIN, GPIO.RISING,
                     callback=wifi_callback,
                     bouncetime=50)

logPrintln("ARC Controller setup complete")

#logPrintln("Waiting for commands...")
logPrintln("Waiting for " + str(delayTime) + " s")

time.sleep(delayTime)
logPrintln("Starting...")
start_recording()

def exit_handler(*args):
	global exited
	if not exited:
		exited = True
		stop_video()
		GPIO.cleanup()
		logPrintln("\nExiting...")
		logFile.close()
		exit(0)

atexit.register(exit_handler)
signal.signal(signal.SIGINT, exit_handler)
signal.signal(signal.SIGTERM, exit_handler)

signal.pause()
