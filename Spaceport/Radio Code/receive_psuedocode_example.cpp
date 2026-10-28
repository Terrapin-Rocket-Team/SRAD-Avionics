#include <RadioLib.h>
#include "RadioMessage.h"
#include "GSInterface.h"

// the same setup as on the transmit side
// this is mostly pulled from the ground station 
Module setup_configuration(ArduinoHalInst, 1, 1, 1, 1) // `new` does not initialize a Module object. It allocates heap memory
    /*Redo based on pins: 
        - Hardware Abstraction Layer (HAL). In this microcontroller this will just be from the arduino
        - SPI Chip select pin
        - Teensy interrupt pin for Arduino
        - Hardware reset pin. By default, this can be RADIOLIB_NC
        - GPIO - Based on module effectiveness 
        - SPI. This is 
    */

SX1281 transceiver(&setup_configuration_goes_here);

ConfigFSK_t *transceiver_controls = ConfigFSK_t(
    2400, // frequency in MHz
    1000, // bitrate, in kpbs
    400, // frequency deviation, in kHz. What's our bandwidth?
    20, // output power in dBm
    10 // preamble length, based on APRSTelem object 
)

// desktop connection stream from teensy to computer
GSInterface gsi(115200);
GSStream telemAvionics = gsi.createStream(APRSTelem::type, TELEM_DEVICE_ID);
GSMessage received;

uint8_t rxBuffer[128];
bool packetReceived = false;

void onPacketReceived() {
    packetReceived = true;
}

void setup() {
    int state = transceiver.beginGFSK(2400.0, 800, 400.0, 10, 16);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("some initialization failure occured");
        return;
    }

    // gsi begin method actually starts sending data over serial
    if (!gsi.begin( (HardwareSerial*)&Serial )) {
        Serial.print("serial couldnt send data");
        return;
    }

    transceiver.setPacketReceivedAction(onPacketReceived); // this radiolib method requires a void function call for received flags
    state = transceiver.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("some error occured when we began receiving");
        return;
    }
}

void loop() {
    // handles the necessary handshakes
    gsi.run();

    // every new iteration we need to reset the received flag's value for the new packet
    packetReceived = false;
    size_t length = transceiver.getPacketLength(); // amount of bytes that arrived

    if (length > 0 && length <= sizeof(rxBuffer)) {
        int state = transceiver.readData(rxBuffer, length); // forward the data we received. look out for error correction in the ground station code
        short rssi = static_cast<short>(transceiver.getRSSI());
        
        if (length != GSMessage::headerLen + received.msgSize) { // without matching headers the rest of the ground station logic into gsmessage wont work
            return;
        }

        received.clear(); // empty out the last gsmessage so we can populate it w/ the new one
        received.fill(rxBuffer, static_cast<uint16_t>(length));

        if (!received.decodeHeader()) {
            Serial.print("unsuccessful decode of packet");
            return;
        }
 
        if (received.dataType != APRSTelem::type || received.id != 2) { // these are example IDs, the real id system is already handled in the ground station, so the id is redundant
            Serial.print("data transmitted was not of type telemetry")
            return;
        }

        APRSTelem telemetry;
        received.decode(&telemetry); // decode the gsmessage and save it into aprstelemetry. this is what the rest of the system will work with!
        gsi.writeStream(&telemetryStream, reinterpret_cast<char*>(received.buf + GSMessage::headerLen), received.msgSize, rssi);
    }

    // Listen for the next packet
    if (transceiver.startReceive() != RADIOLIB_ERR_NONE) {
        stopOnError();
    }
}
