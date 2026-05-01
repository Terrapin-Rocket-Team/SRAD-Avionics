#include <Arduino.h>
#include "RadioMessage.h"

#define SERIAL_BAUD 115200 // bits/s
#define END_CHAR '\n'

HardwareSerial *telemSer = (HardwareSerial *)&Serial;

uint32_t telemTimer = millis();
const uint32_t telemInterval = 100; // ms -> 10 Hz

Message m;
APRSConfig aprscfg = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
APRSTelem telem(aprscfg);

uint8_t stflEncoding[] = {7, 4, 5}; // (Avionics): Temp, stage, fix qual
// uint8_t stflEncoding[] = {7, 5, 8, 7, 4}; // (Airbrake): Temp, flap angle, pred apogee (x2), stage

void setup()
{
    telemSer->begin(SERIAL_BAUD);

    telem.stateFlags.setEncoding(stflEncoding, sizeof(stflEncoding));
}

void loop()
{
    if (millis() - telemTimer > telemInterval)
    {
        telemTimer = millis();

        telem.lat = 0.0;           // decimal latitude
        telem.lng = 0.0;           // decimal longitude
        telem.alt = 0;             // ft
        telem.spd = 0 * 0.5144444; // knots (converted from m/s)
        telem.hdg = 0;             // degree
        telem.orient[0] = 0.0;     // euler angles in degrees (x)
        telem.orient[1] = 0.0;     // euler angles in degrees (y)
        telem.orient[2] = 0.0;     // euler angles in degrees (z)

        // Avionics
        uint8_t temp = 0;    // deg C
        uint8_t stage = 0;   // #
        uint8_t fixQual = 0; // #
        uint8_t flags[] = {temp, stage, fixQual};
        telem.stateFlags.set(flags);

        // Airbrake
        // uint16_t predApogee = 0;                   // ft
        // uint8_t temp = 0;                          // deg C
        // uint8_t flapAng = 0;                       // deg
        // uint8_t predApogee1 = predApogee >> 8;     // (DONT CHANGE)
        // uint8_t predApogee2 = predApogee & 0x00ff; // (DONT CHANGE)
        // uint8_t stage = 0;                         // #
        // uint8_t flags[] = {temp, flapAng, predApogee1, predApogee2, stage};
        // telem.stateFlags.set(flags);

        m.encode(&telem)->print(*telemSer); // automatically terminates with \n
    }
}
