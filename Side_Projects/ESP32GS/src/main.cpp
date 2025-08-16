#include <Arduino.h>
#include <RadioLib.h>

SPIClass spi(HSPI);
//              CS  DIO0  RST   NC     SPI
Module *m = new Module(47, 48, 16, RADIOLIB_NC, spi);
RFM96 radio(m);
#define INITIATING_NODE

int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent or received  packet, set the flag
  operationDone = true;
}

void setup() {
  USBSerial.begin(9600);
  spi.begin(13, 12, 11, /*SS*/ 47);
  delay(5000);
  // initialize SX1278 with default settings
  USBSerial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    USBSerial.println(F("success!"));
  } else {
    USBSerial.print(F("failed, code "));
    USBSerial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when new packet is received
  radio.setDio0Action(setFlag, RISING);

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    USBSerial.print(F("[SX1278] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
    USBSerial.print(F("[SX1278] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      USBSerial.println(F("success!"));
    } else {
      USBSerial.print(F("failed, code "));
      USBSerial.println(state);
      while (true) { delay(10); }
    }
  #endif
}

void loop() {
  // check if the previous operation finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        USBSerial.println(F("transmission finished!"));

      } else {
        USBSerial.print(F("failed, code "));
        USBSerial.println(transmissionState);

      }

      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        USBSerial.println(F("[SX1278] Received packet!"));

        // print data of the packet
        USBSerial.print(F("[SX1278] Data:\t\t"));
        USBSerial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        USBSerial.print(F("[SX1278] RSSI:\t\t"));
        USBSerial.print(radio.getRSSI());
        USBSerial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        USBSerial.print(F("[SX1278] SNR:\t\t"));
        USBSerial.print(radio.getSNR());
        USBSerial.println(F(" dB"));

      }

      // wait a second before transmitting again
      delay(1000);

      // send another one
      USBSerial.print(F("[SX1278] Sending another packet ... "));
      transmissionState = radio.startTransmit("Hello World!");
      transmitFlag = true;
    }
  }
}