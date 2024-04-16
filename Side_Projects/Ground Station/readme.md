# Terp Rockets Ground Station
An [Electron](https://www.electronjs.org/) based ground station user interface to display and log APRS messages recieved over serial

## Installation
The easiest way to install is to download the executable for your platform if it is available for the latest release. However, you can also build the application from source.

First, make sure to install [Node.js](https://nodejs.org/en/) and [npm](https://www.npmjs.com/).

Then install depedencies.
```bash
npm install 
```

If you want to generate an installer in addtion to the basic zip file, add the maker for your platform from makers.txt under config.makers in package.json.
```json
{


"config": {
    "forge": {
      "packagerConfig": {
        "icon": "assets/icon"
      },
      "makers": [
        {
          "name": "@electron-forge/maker-zip"
        }
      ]
    }
  },


}
```

Then make the app using npm.
```bash
npm run make
```

The executable will be located under "out/Terp Rockets Ground Station" and the output from the makers at "out/make/\<name of the maker\>".

## Usage
To get started, first plug in a device that will send messages over serial in the following format:

```javascript
"s\r\nSource:xxx,Destination:xxx,Path:xxx,Type:xxx,Data:xxx,RSSI:xxx\r\ne\r\n"
```
Where
- The x's represent data from the APRS message
- The RSSI must be a number
>**Note**
> If your radio does not give an RSSI value, simply set it to zero

The "Data" field is expected to be in the format:
```javascript
"!DDMM.hhd/DDDMM.hhd[hhh/sss/A=aaaaaa/Sx/HH:MM:SS"
```
Where
- DDMM.hhd is latitude in degrees(DD), minutes(MM.hh), and N or S(d)
- DDDMM.hhd is latatude in the same format but with 3 digits for degrees
- hhh is the azimuth heading in degrees
- sss is the speed in ft/s
- aaaaaa is the altitude in ft (-aaaaaa if negative)
- Sx is the current stage (ex. S0 for stage 0)
- HH:MM:SS is the t0 time in Hours:Minutes:Seconds

### Connecting via the Main Window

To connect the application to the device, select it from the dropdown menu in the application's top bar. If your device is transmitting, you should see data begin to appear. You can check if the device is connected by the plug icon in the top bar. You can also see the signal strength of the receiver by the "wifi" icon in the top bar. The information available in main window is further explained in the user guide.

>**Note**
>If data does not appear, open the debug window using the console icon in the top bar and check for an error message. Make sure the port is not already in use!
>The application's GUI can be reloaded using the reload icon in the top bar.

### Connecting via the Debug Window

The debug window relies on a command based system, similar to that in operating system terminals. To connect to the device from the debug window, use the command

```bash
serial -connect "port-name"
```

Where "port-name" is the name of the serial port (eg. COM5) to connect to without the quotation marks. To see the list of available connections, use the command

```bash
serial -list
```

Other available commands can be seen using the "help" command, and are explained in further detail in the user guide.

### Data
Received data is logged in .csv format and is placed in the /data directory under the name YYYY-MM-DDTHH-MM-SS.csv

## Configuration
Certain application settings can be configured using the settings page (the gear icon in the top bar), commands in the debug window, or by directly editing the config.json file.

The available configuration options are
- the main window scale, in case the main window is too big/small on different resolution screens
- the debug window scale, in case the debug window is too big/small on different resolution screens
- turn on/off debug mode, which will save recieved messages to test.json so they can be used without connecting to the device physically, log debug statements, and open the chromium dev tools used by Electron
- turn on/off noGUI mode, which will open the debug window instead of the main application window on startup
- set the baud rate of the serial port connection for compatibility with different devices
- set the maximum size of the map tile cache so that it does not take up too much storage

## Future development

Features that may be added in the future include
- Integration with live video downlink
- Add APRS IGate capability
- Separate processes so that data will still be logged even if the main application crashes