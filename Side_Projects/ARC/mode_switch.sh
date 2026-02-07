#!/bin/bash

if [ $EUID -ne 0 ]
then
  echo "Error: Please run as root"
  exit 1
fi

if [[ $(nmcli con show ARC-FLIGHT | grep connection.autoconnect: | grep -o yes) == yes ]]
then
	nmcli con modify ARC-FLIGHT connection.autoconnect no
	nmcli con modify FLIGHT-hotspot connection.autoconnect yes
	echo "Switched to host mode"
elif [[ $(nmcli con show FLIGHT-hotspot | grep connection.autoconnect: | grep -o yes) == yes ]]
then
	nmcli con modify ARC-FLIGHT connection.autoconnect yes
        nmcli con modify FLIGHT-hotspot connection.autoconnect no
	echo "Switched to client mode"
fi
