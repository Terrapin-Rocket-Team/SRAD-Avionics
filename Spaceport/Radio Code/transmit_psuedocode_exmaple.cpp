#include <RadioLib.h>
#include "RadioMessage.h"
#include "GSInterface.h"

// PSEUDOCODE - Initialize the SX1281 module w/ the correct pin mappings
Module setup_configuration(ArduinoHalInst, 1, 1, 1, 1) // `new` does not initialize a Module object. It allocates heap memory
    /*Redo based on pins: 
        - Hardware Abstraction Layer (HAL). In this microcontroller this will just be from the arduino
        - SPI Chip select pin
        - Teensy interrupt pin for Arduino
        - Hardware reset pin. By default, this can be RADIOLIB_NC
        - GPIO - Based on module effectiveness 
        - SPI. This is 
    */

// after memory allocation initialize the actual value
// configurations that get passed into the module are specific to the hardware you are working with.
// for some reason, the documentation wants specific initialization methods for the SX127x series. It seems like this isnt necessary w/ SX128x?

// instantiate the transciever's controller
SX1281 transceiver(&setup_configuration_goes_here);

// initialize the transmission parameters
ConfigFSK_t *transceiver_controls_go_here = ConfigFSK_t(
    2400, // frequency in MHz
    1000, // bitrate, in kpbs
    400, // frequency deviation, in kHz. What's our bandwidth?
    20, // output power in dBm
    10 // preamble length, based on APRSTelem object 
)

void setup() {
    int state = transciever.beginGFSK(transceiver_controls_go_here);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("some initialization failure occured");
    }
}

void loop() {
    // handle transmission & receiving in the loop
    // starting w/ forming an APRSTelem object containing our data

    APRSTelem data = APRSTelem(parameters_go_here); // we will start w/ the abstraction that we have sufficient arguments to transmit
    uint8_t packet[56];

    int encoded_length = data.encode(packet, sizeof(packet)); // returns the length of the data we are to transmit

    if (encoded_length < 0) {
        // Encoding failed.
        return;
    }

    // on the reciever end, we will check if our data matches the proper GSMessage formatting. in preperation of that, we must wrap our telemetry before turning to bytes for transmission
    GSMessage message(APRSTelem::type, 2); // passes in the type we expect for accurate parsing when received. 
    message.encode(packet, encoded_length); // copies our actual telemetry and adds the header to it. will save the bytes to transmit into the needed fields

    if (message.hasError()) {
        return;
    }

    int state = transceiver.transmit(message.buf, message.size); // actual transmission code
}
