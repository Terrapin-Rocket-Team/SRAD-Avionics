#include "EncodeAPRSForSerial.h"
#include "Adafruit_BNO055.h"
#include <Arduino.h>
#include "../Radio/RFM69HCW.h"
#include "../Radio/APRS/APRSCmdMsg.h"

#define SERIAL_BAUD 1000000     // 1M Baud

#define PACKET1_SIZE 10000
#define PACKET2_SIZE 10000
#define PACKET3_SIZE 108

byte buff1[PACKET1_SIZE / 8 + 1];
byte buff2[PACKET2_SIZE / 8 + 1];
byte buff3[PACKET3_SIZE / 8 + 1];
byte buff4[PACKET1_SIZE / 8 + 1];

struct RadioObject
{
    APRSTelemMsg *telem;
    APRSCmdMsg *cmd;
    RFM69HCW *radio;
    int packetSize;
    byte *buffer;
    int buffpos;
};

APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1", '/', 'o'};
APRSTelemMsg msg1(header);
APRSCmdMsg cmd1(header);
RadioSettings settings1 = {915.0, 0x02, 0x01, &hardware_spi, 10, 31, 32};
RFM69HCW radio1(&settings1);

APRSTelemMsg msg2(header);
APRSCmdMsg cmd2(header);
RadioSettings settings2 = {915.0, 0x02, 0x01, &hardware_spi, 10, 31, 32};
RFM69HCW radio2(&settings2);

// Let radio3 be the telemetry receiver
APRSTelemMsg msg3(header);
APRSCmdMsg cmd3(header);
RadioSettings settings3 = {433.775, 0x02, 0x01, &hardware_spi, 10, 31, 32};
RFM69HCW radio3(&settings3);

RadioObject r1 = {&msg1, &cmd1, &radio1, PACKET1_SIZE, buff1, 0};
RadioObject r2 = {&msg2, &cmd2, &radio2, PACKET2_SIZE, buff2, 0};
RadioObject r3 = {&msg3, &cmd3, &radio3, PACKET3_SIZE, buff3, 0};

// make an array of pointers to the radio objects
RadioObject *radios[] = {&r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r3};

void setup()
{
    delay(2000); // Delay to allow the serial monitor to connect
    if (!radio1.init())
        Serial.println("Radio1 failed to initialize");
    else
        Serial.println("Radio1 initialized");

    if (!radio2.init())
        Serial.println("Radio2 failed to initialize");
    else
        Serial.println("Radio2 initialized");

    if (!radio3.init())
        Serial.println("Radio3 failed to initialize");
    else
        Serial.println("Radio3 initialized");

    Serial1.begin(SERIAL_BAUD);

        // Test Case for Demo Purposes
    cmd3.data.MinutesUntilPowerOn = 0;
    cmd3.data.MinutesUntilDataRecording = 1;
    cmd3.data.MinutesUntilVideoStart = 2;
    cmd3.data.Launch = false;
}


int counter = 1;
int radioIndex = 0;
void loop()
{

    if (radios[radioIndex]->radio->update())
    {
        if (radios[radioIndex]->radio->dequeueReceive(radios[radioIndex]->telem))
        {
            char buffer[radios[radioIndex]->packetSize];
            aprsToSerial::encodeAPRSForSerial(*(radios[radioIndex]->telem), buffer, radios[radioIndex]->packetSize, radios[radioIndex]->radio->RSSI());
            Serial.println(buffer);
        }
        if(counter % 50 == 0) // Send a command every 50x a message is received (or every 25 seconds)
            radios[radioIndex]->radio->enqueueSend(radios[radioIndex]->cmd);

        if(counter == 190) // Send a command after 190x a message is received (or after 95 seconds) to launch the rocket
        {
            radios[radioIndex]->cmd->data.Launch = !radios[radioIndex]->cmd->data.Launch;
            radios[radioIndex]->radio->enqueueSend(radios[radioIndex]->cmd);
        }
        counter++;
    }

    // if (radio.update())
    // {
    //     if (radio.dequeueReceive(&msg))
    //     {
    //         char buffer[255];
    //         aprsToSerial::encodeAPRSForSerial(msg, buffer, 255, radio.RSSI());
    //         Serial.println(buffer);
    //     }
    //     if(counter % 50 == 0) // Send a command every 50x a message is received (or every 25 seconds)
    //         radio.enqueueSend(&cmd);

    //     if(counter == 190) // Send a command after 190x a message is received (or after 95 seconds) to launch the rocket
    //     {
    //         cmd.data.Launch = !cmd.data.Launch;
    //         radio.enqueueSend(&cmd);
    //     }
    //     counter++;
    // }
}