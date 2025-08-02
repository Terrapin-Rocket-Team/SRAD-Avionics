#include <Arduino.h>
#include <RadioLib.h>
//ESP32 GROUND STATION

#define ADC 10;
// PIN CONFIGS

/*
NCS: 14
IRQ: 36 (RAD_IO9)!!!
NRST: 38
BUSY: 37
*/
SPIClass spi = SPIClass(HSPI);
Module *module = new Module(14, 36, 38, 37, spi); // CS, IRQ/DIO9, NRST, BUSY
LR1121 radio = LR1121(module);
void setup()
{
    USBSerial.begin(115200);
    delay(5000);
    USBSerial.println("Starting LR1121...");
    spi.begin(13, 12, 11, 14); // SCK, MISO, MOSI, CS
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE)
        USBSerial.println(F("success!"));

    pinMode(15, INPUT_PULLUP); // DIO2
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);
}
int count = 0;
void loop()
{

    delay(1000);

    count++;
    // String str = "Hello World! #" + String(count++);
    // int state = radio.transmit(str);
    USBSerial.println(digitalRead(15));
    USBSerial.println(digitalRead(21));
    USBSerial.println("Count: " + String(count));




}