# Apollo    

Welcome to the Terrapin Rocket Team (TRT). Project Apollo is a TOP SECRET mission with the goal of developing cutting edge technology for the scientific community. You have been asked to create a High Fidelity Data Logging Flight Computer for an L1 High Power Rocket (HFDLFCL1HPR). The instruments you will create will progress the aerospace industry and bring us closer to coninuous pressence on the Lunar surface and beyond... 

Your selection for this opportunity is a testament to your capabilities. Over the next several weeks, you will work on a multidiplinary team to research, design, test and integrate the next generation of SRAD (student researched and developed) avionics. 

## Design Requirements

Our contact at NASA Goddard Space Flight Center has requested that you make a PCB (printed circuit board) to record data during the flight of a small scale sounding rocket. NASA provided us with sensor hardware you will have to integrate. 

Sensor Hardware:
- Barometers:
    - DPS368
    - DPS310
    - MS5611
    - BMP390
    - BMP280
    
- GPS modules:
    - Sparkfun NEO-M9N-00B (SMA)
    - Sparkfun NEO-M9N-00B (chip antenna)
    - TRT proprietary GPS PCB (SMA)

PCB requirements:

- Measure alititude based on barometric pressure
- Footprint for a through-hole Teensy 4.1 microcontroller (MCU)
- Visually indicate system status on the PCB using LEDs
- Power from 4.8V battery 
- Include SMA connectivity for GPS
- I2C breakout 
- UART breakout
- SPI breakout

## Project Objectives

    1. Become familiar with the PCB schematic and layout design process
    2. Develop expertise in integrating sensor hardware with different communication protocols (I2C, UART, SPI)
    3. Create a reliable data logging program with TRT Multi Mission Flight Software (MMFS) libraries
    4. Understand what goes in to creating an embedded system 

## Development Strategy

Phase 1, Research:
- Create a bill of materials BOM for your flight computer. This should include all components of the PCB including sensors, resistors capacitors and MCUs reguardless of whether they are present in the lab or need to be ordered.

- Read the datasheet for sensor hardware that will be integrated. Look carefully for the communication protocol the deviceswill use to communicate to other devices such as the MCU. 
    - Take note of which pins are required for that protocol for the next phase:

Phase 2, PCB Design:

- Generate a schematic in KiCad. This is a theoretical diagram that details the interconnects between devices. This is also where you will decide where to put LEDs or any other functionality to the board. 

- Assign footprints to symbols. Each symbol in the schematic will need an associated footprint. An SMD footprint is the arrangement of copper pads and outlines on a PCB designed to physically and electrically connect a surface-mount component. Unlike through-hole components, SMD parts sit directly on the board and are soldered to pads rather than inserted into drilled holes.
    - Your board will also need a footprint for a through-hole Teensy4.1 MCU. This will allow you to hand solder the MCU to your computer. 

- Complete the PCB layout and routing. This is where you will design the physical implementation of your schematic. PCB routing is the process of designing the copper traces that electrically connect components on a PCB. Layout refers to the overall arrangement of components and routing to ensure proper functionality, signal integrity, and manufacturability. 

Each sensor such as a barometer or GPS module interfaces may with the Teensy 4.1 MCU with a different communication protocol. Each protocol has its own set of rules that promote interoperability between devices in an embedded system. Start by reading the datasheet for your sensor to determine its capabilities and start to think about how you can use it for this project. 

KiCad is the primary tool you will use to generate the designs for your custom PCB. Bellow are some links to tutorials to get you started. 
