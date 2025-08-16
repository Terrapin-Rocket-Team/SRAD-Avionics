#include <RadioLib.h>
SPIClass spi(HSPI);
LR1121 radio = new Module(14, 36, 38, 37, spi);

static const uint32_t rfswitch_dio_pins[] = {
    RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
    RADIOLIB_LR11X0_DIO7, RADIOLIB_NC, RADIOLIB_NC};

static const Module::RfSwitchMode_t rfswitch_table[] = {
    // mode                  DIO5  DIO6  DIO7
    {LR11x0::MODE_STBY, {LOW, LOW, LOW}},
    {LR11x0::MODE_RX, {LOW, LOW, HIGH}},
    {LR11x0::MODE_TX, {LOW, HIGH, LOW}},
    {LR11x0::MODE_TX_HP, {HIGH, LOW, LOW}},
    END_OF_MODE_TABLE,
};

// save transmission states between loops
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
  // we sent or received a packet, set the flag
  operationDone = true;
}

void setup() {
  USBSerial.begin(9600);
  delay(5000);
  spi.begin(13, 12, 11, /*SS*/ 14);

  // initialize LR1110 with default settings
  USBSerial.print(F("[LR1110] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    USBSerial.println(F("success!"));
  } else {
    USBSerial.print(F("failed, code "));
    USBSerial.println(state);
    while (true) { delay(10); }
  }

  // set RF switch control configuration
  radio.setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);

  radio.setRegulatorDCDC();

  // set the function that will be called
  // when new packet is received
  radio.setIrqAction(setFlag);

  #if defined(INITIATING_NODE)
    // send the first packet on this node
    USBSerial.print(F("[LR1110] Sending first packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
  #else
    // start listening for LoRa packets on this node
    USBSerial.print(F("[LR1110] Starting to listen ... "));
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
        USBSerial.println(F("[LR1110] Received packet!"));

        // print data of the packet
        USBSerial.print(F("[LR1110] Data:\t\t"));
        USBSerial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        USBSerial.print(F("[LR1110] RSSI:\t\t"));
        USBSerial.print(radio.getRSSI());
        USBSerial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        USBSerial.print(F("[LR1110] SNR:\t\t"));
        USBSerial.print(radio.getSNR());
        USBSerial.println(F(" dB"));

      }

      // wait a second before transmitting again
      delay(1000);

      // send another one
      USBSerial.print(F("[LR1110] Sending another packet ... "));
      transmissionState = radio.startTransmit("Hello World!");
      transmitFlag = true;
    }
  
  }
}