#!/bin/bash

# $1 = ip to stream to
# $2 = port to stream to

if [ $# -eq 0 ] 
then
    echo "Error: no arguments given"
    echo $0 "target_ip target_port" 
    exit 1
fi

rpicam-vid -t 0 --buffer-count 12 --codec yuv420 --width 1280 --height 720 --framerate 30 --no-raw --flush -o - | aomenc -w 1280 -h 720 --profile=0 --kf-max-dist=300 --end-usage=cbr --min-q=1 --max-q=38 --undershoot-pct=100 --overshoot-pct=25 --buf-sz=20 --buf-initial-sz=10 --buf-optimal-sz=10 --max-intra-rate=600 --passes=1 --lag-in-frames=0 --error-resilient=0 --tile-columns=1 --tile-rows=3 --aq-mode=2 --enable-obmc=0 --enable-global-motion=1 --enable-warped-motion=0 --deltaq-mode=0 --enable-tpl-model=1 --mode-cost-upd-freq=2 --coeff-cost-upd-freq=2 --enable-ref-frame-mvs=0 --mv-cost-upd-freq=3 --enable-order-hint=0 --drop-frame=0 --resize-mode=3 --cpu-used=5 --rt --usage=1 --threads=3 --obu --target-bitrate=450 --fps=30/1 - -o - | nc $1 $2
