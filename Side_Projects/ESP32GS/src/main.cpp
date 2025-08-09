#include <Arduino.h>
#include <RadioLib.h>

SPIClass spi(HSPI);
//              CS  DIO0  RST   NC     SPI
Module *m = new Module(47, 48, 16, RADIOLIB_NC, spi);
RFM96 radio(m);

void setup()
{
    USBSerial.begin(115200);
    delay(200);
    spi.begin(/*SCK*/ 13, /*MISO*/ 12, /*MOSI*/ 11, /*SS*/ 47);

    int s = radio.begin();
    USBSerial.printf("RFM96 begin: %d\n", s);

    // LoRa PHY that must MATCH the LR1121 side
    radio.setFrequency(915.0);
    radio.setBandwidth(125.0);   // kHz
    radio.setSpreadingFactor(7); // SF7
    radio.setCodingRate(5);      // 4/5
    radio.setPreambleLength(8);
    radio.setCRC(true);
    radio.setSyncWord(0x34); // “public” LoRa sync word
    radio.explicitHeader();
    radio.invertIQ(false); // public sync
}

void loop()
{

    
    // String rx;
    // int s = radio.receive(rx); // blocking; fine for smoke test
    // if (s == RADIOLIB_ERR_NONE)
    // {
    //     USBSerial.printf("RX ok: %s  RSSI=%.1f  SNR=%.1f\n",
    //                      rx.c_str(), radio.getRSSI(), radio.getSNR());
    // }
    // else if (s == RADIOLIB_ERR_RX_TIMEOUT)
    // {
    //     // no packet this cycle; you can ignore or retry
    // }
    // else
    // {
    //     USBSerial.printf("RX err: %d\n", s);
    // }

    int ch = radio.scanChannel();  // 1 if activity/preamble detected, 0 if clear, <0 error
USBSerial.printf("scanChannel: %d  RSSI=%.1f\n", ch, radio.getRSSI());
}
