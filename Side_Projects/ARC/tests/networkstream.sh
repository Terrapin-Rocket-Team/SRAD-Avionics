#!/bin/bash

# $1 = ip to stream to
# $2 = port to stream to

if [ $# -eq 0 ] 
then
    echo "Error: no arguments given"
    echo $0 "target_ip target_port" 
    exit 1
fi

rpicam-vid -t 0 --buffer-count 6 --width 1280 --height 720 --framerate 60 --nopreview --no-raw --flush -o - | nc $1 $2
