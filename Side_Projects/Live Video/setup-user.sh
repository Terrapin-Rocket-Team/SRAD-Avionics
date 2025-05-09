#!/bin/bash

# create proper service file
sed "s|USER_SCRIPT|$HOME|g" ARC.service > ARC.service.temp
sed -i "s|USER_NAME|$USER|g" ARC.service.temp

# move controller script to correct location
cp ARC_controller.py ~/ARC_controller.py

# make video folder
mkdir -p ~/ARC_video
chmod +rwx ~/ARC_video
# make log folder
mkdir -p ~/ARC_log
chmod +rwx ~/ARC_log