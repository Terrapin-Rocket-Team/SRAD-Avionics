#include "EncodeAPRSForSerial.h"
#include "Adafruit_BNO055.h"
#include <Arduino.h>
#include "../Radio/RFM69HCW.h"
#include "../Radio/APRS/APRSCmdMsg.h"

APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1", '/', 'o'};
APRSTelemMsg msg(header);
APRSCmdMsg cmd(header);
RadioSettings settings = {915.0, 0x02, 0x01, &hardware_spi, 10, 31, 32};
RFM69HCW radio(&settings);


void setup()
{
    delay(2000); // Delay to allow the serial monitor to connect
    if (!radio.init())
        Serial.println("Radio failed to initialize");
    else
        Serial.println("Radio initialized");

        // Test Case for Demo Purposes
    cmd.data.MinutesUntilPowerOn = 0;
    cmd.data.MinutesUntilDataRecording = 1;
    cmd.data.MinutesUntilVideoStart = 2;
    cmd.data.Launch = false;
}


int counter = 1;
void loop()
{
    if (radio.update())
    {
        if (radio.dequeueReceive(&msg))
        {
            char buffer[255];
            aprsToSerial::encodeAPRSForSerial(msg, buffer, 255, radio.RSSI());
            Serial.println(buffer);
        }
        if(counter % 50 == 0) // Send a command every 50x a message is received (or every 25 seconds)
            radio.enqueueSend(&cmd);

        if(counter == 190) // Send a command after 190x a message is received (or after 95 seconds) to launch the rocket
        {
            cmd.data.Launch = !cmd.data.Launch;
            radio.enqueueSend(&cmd);
        }
        counter++;
    }
}