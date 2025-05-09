#!/bin/python -u

import RPi.GPIO as GPIO
from datetime import datetime
import subprocess
import time
import os
import re

# GPIO Configuration
CMD_PIN = 6    # Input pin (from Teensy)
INTERFACE_PIN = 12 # Input pin (external source)
# RESP_PIN = 5   # Output pin (to Teensy)
GPIO.setmode(GPIO.BCM)
GPIO.setup(CMD_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
# GPIO.setup(RESP_PIN, GPIO.OUT, initial=GPIO.HIGH)
unixTime = int((datetime.now() - datetime(1970, 1, 1)).total_seconds())

logFile = open(os.path.expanduser("~/ARC_log/log_" + str(unixTime) + ".txt"), "w")

# handy logging functions
def logPrintln(str):
    str = "[" + datetime.now().time() + "]" + str
    print(str)
    logFile.write(str + "\n")
    logFile.flush()


logPrintln("ARC Controller startup complete")

# process control
video = False
live = False
interface = False

# subprocesses
rpicam_process = None
av1_process = None
transmit_process = None
interface_process = None

# command strings
command_rpicam = "rpicam-vid -t 300000 --codec yuv420 --width 1280 --height 720 --framerate 30 --no-raw -o -"
command_av1 = "aomenc -w 1280 -h 720 --profile=0 --kf-max-dist=300 --end-usage=cbr " \
    "--min-q=1 --max-q=52 --undershoot-pct=100 --overshoot-pct=25 --buf-sz=20 --buf-initial-sz=10" \
    " --buf-optimal-sz=10 --max-intra-rate=600 --passes=1 --lag-in-frames=0 --error-resilient=0 " \
    "--tile-columns=1 --tile-rows=3 --aq-mode=2 --enable-obmc=0 --enable-global-motion=1 " \
    "--enable-warped-motion=0 --deltaq-mode=0 --enable-tpl-model=1 --mode-cost-upd-freq=2 " \
    "--coeff-cost-upd-freq=2 --enable-ref-frame-mvs=0 --mv-cost-upd-freq=3 --enable-order-hint=0 " \
    "--drop-frame=0 --resize-mode=3 --cpu-used=9 --rt --usage=1 --threads=3 --obu --target-bitrate=400 " \
    "--fps=30/1 - -o " + os.path.expanduser("~/ARC_video/video_" + str(unixTime) + ".av1")
command_transmit = "teensy-interface"
command_interface = "node ARC_interface/app.js"

def start_recording():
    global rpicam_process
    global av1_process
    global command_rpicam
    global command_av1

    rpicam_process = subprocess.Popen(command_rpicam.split(" "),
                                    stdout=subprocess.PIPE)
    av1_process = subprocess.Popen(command_av1.split(" "),
                                    stdin=rpicam_process.stdout)
    
def start_transmitting():
    global rpicam_process
    global av1_process
    global transmit_process
    global command_rpicam
    global command_av1
    global command_transmit

    rpicam_process = subprocess.Popen(command_rpicam.split(" "),
                                    stdout=subprocess.PIPE)
    av1_process = subprocess.Popen(command_av1.split(" "),
                                    stdin=rpicam_process.stdout, stdout=subprocess.PIPE)
    transmit_process = subprocess.Popen(command_transmit.split(" "), stdin=av1_process.stdout)

def stop_video():
    global rpicam_process
    global av1_process
    global transmit_process
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

def video_callback(channel):
    global video
    cmd_state = GPIO.input(CMD_PIN)

    if cmd_state == GPIO.LOW and not video:
        # Start recording
        if not live:
            start_recording()
        if live:
            start_transmitting()
        video = True
        # GPIO.output(RESP_PIN, GPIO.LOW)
        logPrintln("Recording started")

    elif cmd_state == GPIO.HIGH and video:
        # Stop recording
        stop_video()
        video = False
        # GPIO.output(RESP_PIN, GPIO.HIGH)
        logPrintln("Recording stopped")

def start_interface():
    global interface_process
    global command_interface

    interface_process = subprocess.Popen(command_interface.split(" "), stdout=subprocess.PIPE)

# string to indicate a command from the interface
cmd_trigger = "PYCMD"
# separator between trigger and command
cmd_sep = ":"
# list of possible commands and their corresponding callbacks
cmd_list = ["start recording", "start transmitting", "stop video"]
cmd_callbacks = [start_recording, start_transmitting, stop_video]

def process_interface():
    global interface_process
    global cmd_trigger
    global cmd_sep
    global cmd_list
    global cmd_callbacks

    # get a line
    line = interface_process.stdout.readline()
    if line:
        # check for cmd_trigger and sep
        m = re.match(cmd_trigger + cmd_sep + ".+", line)
        if m:
            # if match get command part (after sep character)
            cmd = m.group().split(cmd_sep)[1].trim()
            # find correct callback
            for i in range(len(cmd_list)):
                if cmd_list[i] == cmd:
                    cmd_callbacks[i]()
                    break

        

def stop_interface():
    global interface_process
    if interface_process:
        interface_process.kill()
        interface_process.wait()
        interface_process = None

def interface_callback(channel):
    global interface
    cmd_state = GPIO.input(CMD_PIN)

    if cmd_state == GPIO.LOW and not interface:
        start_interface()
        interface = True
        logPrintln("Started ARC interface")
    elif cmd_state == GPIO.HIGH and interface:
        stop_interface()
        interface = False
        logPrintln("Stopped ARC interface")

# Add interrupt detection for both edges
GPIO.add_event_detect(CMD_PIN, GPIO.BOTH,
                     callback=video_callback,
                     bouncetime=50)

# Add interrupt detection for both edges
GPIO.add_event_detect(INTERFACE_PIN, GPIO.BOTH,
                     callback=interface_callback,
                     bouncetime=50)

logPrintln("ARC Controller setup complete")

try:
    logPrintln("Waiting for commands...")
    while True:
        if interface:
            process_interface()
        time.sleep(1)
except KeyboardInterrupt:
    stop_video()
    GPIO.cleanup()
    logPrintln("\nExiting...")
    logFile.close()