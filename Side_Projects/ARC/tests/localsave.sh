#!/bin/bash

libcamera-vid -t 0 --codec yuv420 --width 1280 --height 720 -o - | ffmpeg -s 1280x720 -f rawvideo -i - -vcodec h264_v4l2m2m -r 30 -b:v 5M -an -sn -dn -y ~/video/test.mp4