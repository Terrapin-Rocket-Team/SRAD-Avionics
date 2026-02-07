#!/bin/bash

echo "Getting ARC files..."
cd ~
git clone --depth 1 --no-single-branch --no-checkout https://github.com/Terrapin-Rocket-Team/SRAD-Avionics.git

cd SRAD-Avionics

git sparse-checkout set Side_Projects/ARC

git checkout main

echo "Creating symlink to ~/ARC..."
ln -s ~/SRAD-Avionics/Side_Projects/ARC ~/ARC
chmod +x ~/ARC/setup.sh
echo "Done"
echo "Remember to run \"cd ~/ARC && sudo ./setup.sh\""
