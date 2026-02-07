#!/bin/bash

GET_AV1=0

echo " --------------------------------------------------"
echo " | Starting ARC Setup Script v2.0                 |"
echo " --------------------------------------------------"

if [ $EUID -ne 0 ]
then 
  echo "Error: Please run as root"
  exit 1
fi

echo "Checking dependencies..."
sleep 1

echo -n "Checking for git..."
if ! type git &> /dev/null
then
    echo "Not Found"
    echo "Installing git..."
    apt install git -y
else
    echo "Found"
fi

echo -n "Checking for cmake..."
if ! type cmake &> /dev/null
then
    echo "Not Found"
    echo "Installing cmake..."
    apt install cmake -y
else
    echo "Found"
fi

echo -n "Checking for ffmpeg..."
if ! type ffmpeg &> /dev/null
then
    echo "Not Found"
    echo "Installing ffmpeg..."
    apt install ffmpeg -y
else
    echo "Found"
fi

echo -n "Checking for perl..."
if ! type perl &> /dev/null
then
    echo "Not Found"
    echo "Installing perl..."
    apt install perl -y
else
    echo "Found"
fi

echo -n "Checking for rpicam..."
if ! type rpicam-vid &> /dev/null
then
    echo "Not Found"
    echo "Installing rpicam..."
    apt install rpicam-apps -y
else
    echo "Found"
fi

echo -n "Checking for netcat..."
if ! type nc &> /dev/null
then
    echo "Not Found"
    echo "Installing netcat..."
    apt install netcat-traditional -y
else
    echo "Found"
fi

echo -n "Checking for node..."
if ! type node &> /dev/null
then
    echo "Not Found"
    echo "Installing node..."
    apt install nodejs -y
else
    echo "Found"
fi

echo -n "Checking for wiringPi..."
if [ ! -f /usr/include/wiringPi.h ]
then
    echo "Not Found"
    echo "Installing wiringPi..."
    curl -LO https://github.com/WiringPi/WiringPi/releases/download/3.4/wiringpi_3.4_arm64.deb
    apt install ./wiringpi_3.4_arm64.deb
    rm wiringpi_3.4_arm64.deb
else
    echo "Found"
fi

if [[ $GET_AV1 -eq 1 ]]
then
echo "Getting AV1 encoder..."
sleep 1

if [ -d aom ]
then
    cd aom
    if [ ! git rev-parse --is-inside-work-tree &> /dev/null ]
    then
        cd ..
        rm -rf aom
        git clone https://aomedia.googlesource.com/aom
    else
        cd ..
        echo "Found git repository, assuming it is the right one"
    fi
else
    git clone https://aomedia.googlesource.com/aom
fi

cd aom
mkdir -p build && cd build

echo "Building AV1 encoder..."
sleep 1

cmake ..
make

echo "Installing encoder..."
sleep 1

make install

cd ../../
fi

echo "Building teensy interface..."
sleep 1

cd teensy-interface
make

echo "Finishing up..."
sleep 1

cd ../

chmod +x tests/*
chmod +x ARC_controller.py
chmod +x setup-user.sh

# add test wifi
if [[ $(nmcli con show | grep -o ARC-1) != "ARC-1" ]]
then
nmcli connection add type wifi con-name ARC-1 autoconnect yes ssid TRT-ARC-1 connection.autoconnect-priority 10 wifi-sec.key-mgmt WPA-PSK wifi-sec.psk IREC2025!
fi
if [[ $(nmcli con show | grep -o ARC-2) != "ARC-2" ]]
then
nmcli connection add type wifi con-name ARC-2 autoconnect yes ssid TRT-ARC-2 connection.autoconnect-priority 10 wifi-sec.key-mgmt WPA-PSK wifi-sec.psk IREC2025!
fi
#nmcli connection add type wifi con-name ARC-3 autoconnect yes ssid TRT-ARC-3 connection.autoconnect-priority 10 wifi-sec.key-mgmt WPA-PSK wifi-sec.psk IREC2025!

# add intra-rocket host hotspot
if [[ $(nmcli con show | grep -o FLIGHT-hotspot) != "FLIGHT-hotspot" ]]
then
# autoconnect no to initally set all pi's up in client mode
nmcli connection add type wifi ifname wlan0 con-name FLIGHT-hotspot autoconnect no ssid TRT-ARC-FLIGHT connection.autoconnect-priority 9
nmcli connection modify FLIGHT-hotspot 802-11-wireless.mode ap 802-11-wireless.band bg ipv4.method shared
nmcli connection modify FLIGHT-hotspot wifi-sec.key-mgmt wpa-psk wifi-sec.psk IREC2025!
fi

# add intra-rocket client wifi
if [[ $(nmcli con show | grep -o ARC-FLIGHT) != "ARC-FLIGHT" ]]
then
nmcli connection add type wifi con-name ARC-FLIGHT autoconnect yes ssid TRT-ARC-FLIGHT connection.autoconnect-priority 8 wifi-sec.key-mgmt WPA-PSK wifi-sec.psk IREC2025!
fi
sudo -u ${SUDO_USER} bash -c "./setup-user.sh"

chmod 644 ARC.service.temp
mv ARC.service.temp /lib/systemd/system/ARC.service
systemctl daemon-reload
systemctl enable ARC
systemctl start ARC

echo "Setup complete!"
