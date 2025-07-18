# Project Apollo    

Welcome to the Terrapin Rocket Team. Project Apollo is a TOP SECRET mission with the goal of developing cutting edge technology for the scientific community. You have been asked to create a High Fidelity Data Logging Flight Computer for an L1 High Power Rocket (HFDLFCL1HPR). The instruments you will create will progress the aerospace industry and bring us closer to coninuous pressence on the Lunar surface and beyond... 

Your selection for this opportunity is a testament to your capabilities. Over the next several weeks, you will work on a multidiplinary team to research, design, test and integrate the next generation of SRAD (student researched and developed) avionics. 

## Design Requirements

Our contact at NASA Goddard Space Flight Center has requested that you make a PCB (printed circuit board) to record data during the flight of a small scale sounding rocket. NASA provided us with sensor hardware you will have to integrate. In other words, they aren't quite sure how it works. That is why you are here.

Sensor hardware:
- Barometers:
    - DPS368
    - DPS310
    - MS5611
    - BMP390
    - BMP280
- GPS modules:
    - TBD patch breakout

PCB requirements:
- Measure alititude based on barometric pressure
- Solder a through-hole Teensy 4.1 μController
- Visually indicate system status 
- Power from 4.8V battery
- Include connectivity for GPS
- 