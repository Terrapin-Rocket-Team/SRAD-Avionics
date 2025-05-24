#!/bin/bash

if [ ! -f /home/lvpi1/video/test2.mp4 ] && [ -f /home/lvpi1/video/test1.mp4 ] && [ -f /home/lvpi1/video/test.mp4 ]
then
   timeout 5m libcamera-vid -t 0 --no-raw --codec yuv420 --width 1920 --height 1080 --framerate 30 -o - | ffmpeg -s 1920x1080 -f rawvideo -pix_fmt yuv420p -i - -vcodec h264_v4l2m2m -r 30 -b:v 2M -an -sn -dn -y /home/lvpi1/video/test2.mp4
fi
if [ ! -f /home/lvpi1/video/test1.mp4 ] && [ -f /home/lvpi1/video/test.mp4 ]
then
   timeout 5m libcamera-vid -t 0 --no-raw --codec yuv420 --width 1920 --height 1080 --framerate 30 -o - | ffmpeg -s 1920x1080 -f rawvideo -pix_fmt yuv420p -i - -vcodec h264_v4l2m2m -r 30 -b:v 2M -an -sn -dn -y /home/lvpi1/video/test1.mp4
fi
if [ ! -f /home/lvpi1/video/test.mp4 ]
then
   timeout 5m libcamera-vid -t 0 --no-raw --codec yuv420 --width 1920 --height 1080 --framerate 30 -o - | ffmpeg -s 1920x1080 -f rawvideo -pix_fmt yuv420p -i - -vcodec h264_v4l2m2m -r 30 -b:v 2M -an -sn -dn -y /home/lvpi1/video/test.mp4
fi
sleep 120
shutdown -h now
