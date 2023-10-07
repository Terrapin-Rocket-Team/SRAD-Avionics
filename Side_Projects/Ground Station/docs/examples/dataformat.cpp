/*===================================================================================================
--------------------------------------------------------------------
Example Code for Communicating with the Ground Station Over Serial:
--------------------------------------------------------------------

Note that this code will not compile on its own. Rather this specific implementation is one of many,
and is meant to show only one example of how to communicate with the ground station over Serial.

The placeholder functions are only to show what the code would generally be doing, and would need
to be replaced with your own code to function properly.

The structure and syntax of this example is based on Arduino and the APRS-Decoder library.

====================================================================================================*/

#include <Arduino.h>
#include <APRS-Decoder.h>

// object to parse the APRS message
APRSMessage msg;
// sends the recieved message to the ground station over serial
void sendData(String data, int rssi);

// setup function, runs once
void setup()
{
    // activate the serial connection with a baud rate of 115200
    Serial.begin(115200);
}

// loop function, runs continuously after setup()
void loop()
{
    // placeholder function to check if data has been recieved
    if (hasRecievedRadioData())
    {
        // placeholder function to get the APRS transmission as a string
        String recv = getRadioAPRSMessage();
        // placeholder function to get the rssi of the transmission
        int rssi = getMessageRssi();

        // pass the message and rssi to be sent to the ground station
        sendData(recv, rssi);
    }
}

// sendData function
void sendData(String data, int rssi)
{
    // decode the APRS message using the APRSMessage object
    msg.decode(data);
    // get the decoded APRS messsage as a string, this is in the form "Source:xxxXX, Destination:xxx, Path:xxx, Type:xxx, Data:xxx"
    String str = msg.toString();
    // remove the spaces between fields
    str.replace(" ", "");
    // get rid of the extra characters (represented by XX) that are added to the end of the callsign
    str = str.substring(str.indexOf("Source:"), str.indexOf("Source:") + 7) + str.substring(str.indexOf("Source:") + 10);
    // send the message over serial, adding the RSSI and correct formatting
    // we want to use print() rather than println() here because we don't want any extra newline characters
    Serial.print("s\r\n" + str + ",RSSI:" + (String)rssi + "\r\ne\r\n");
}