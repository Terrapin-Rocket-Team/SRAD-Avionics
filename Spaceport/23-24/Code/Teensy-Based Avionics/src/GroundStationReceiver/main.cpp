#include "EncodeAPRSForSerial.h"
#include "Adafruit_BNO055.h"
#include <Arduino.h>
#include "../Radio/RFM69HCW.h"
#include "../Radio/APRS/APRSCmdMsg.h"
#include "../LiveRadio/LiveRFM69HCW.h"

#define SERIAL_BAUD 1000000     // 1M Baud => 8us per byte

// in bytes
#define PACKET1_SIZE 10000 / 8
#define PACKET2_SIZE 10000 / 8
#define PACKET3_SIZE 63

// makes them 4 times as large
#define BUFF1_SIZE PACKET1_SIZE * 4
#define BUFF2_SIZE PACKET2_SIZE * 4
#define BUFF3_SIZE PACKET3_SIZE * 4
#define BUFF4_SIZE PACKET3_SIZE * 4

byte buff1[BUFF1_SIZE];
byte buff2[BUFF2_SIZE];
byte buff3[BUFF3_SIZE];
byte buff4[BUFF4_SIZE];

// live video stuff 

#define MSG_SIZE 25000
#define TX_ADDR 0x02
#define RX_ADDR 0x01
#define SYNC1 0x00
#define SYNC2 0xff

void radioIdle(RadioObject *rad);
void radioRx(RadioObject *rad);
void readLiveRadio(RadioObject *rad);

struct LiveSettings {
    bool hasTransmission;
    bool modeReady;
    bool modeIdle = true;
    unsigned int pos;
    bool hasL;
    bool hasA;
};

struct RadioObject
{
    APRSTelemMsg *telem;
    APRSCmdMsg *cmd;
    RFM69HCW *radio;
    const int packetSize;
    byte *buffer;
    int bufftop;        // index of the next byte to be written
    int buffbot;        // index of the next byte to be read
    LiveSettings settings;
};

// just for sending info
int bufftop4 = 0;
int buffbot4 = 0;

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

LiveSettings lset1 = {false, false, true, 0, false, false};
LiveSettings lset2 = {false, false, true, 0, false, false};
RadioObject r1 = {&msg1, &cmd1, &radio1, PACKET1_SIZE, buff1, 0, 0, lset1};
RadioObject r2 = {&msg2, &cmd2, &radio2, PACKET2_SIZE, buff2, 0, 0, lset2};
RadioObject r3 = {&msg3, &cmd3, &radio3, PACKET3_SIZE, buff3, 0, 0};

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

    // sdjkfhkjsd(&r3);
}


int counter = 1;
int radioIndex = 0;
void loop()
{

    if (radio1.update())
    {
        if (radio1.dequeueReceive(&msg1))
        {
            char buffer[80];
            aprsToSerial::encodeAPRSForSerial(msg1, buffer, 80, radio1.RSSI());
            
            // write to circular buffer for radio3 (assuming 63 bytes of data)
            for (int i = 0; i < 63; i++)
            {
                buff3[r3.bufftop] = buffer[i];
                r3.bufftop = (r3.bufftop + 1) % BUFF3_SIZE;
            }

        }
        // if(counter % 50 == 0) // Send a command every 50x a message is received (or every 25 seconds)
        //     radio.enqueueSend(&cmd);

        // if(counter == 190) // Send a command after 190x a message is received (or after 95 seconds) to launch the rocket
        // {
        //     cmd.data.Launch = !cmd.data.Launch;
        //     radio.enqueueSend(&cmd);
        // }
        // counter++;
    }
    if (radio2.update()) {
        const char *msg = radio2.rxX();

        // write to circular buffer of radio 2
        // get the length of the message
        int lens = strlen(msg);         // fix bc idk message length since its char array and not null terminated
    }
}

// adapted from radioHead functions
void radioIdle(RadioObject *rad)
{
    rad->settings.modeReady = false;
    rad->settings.modeIdle = true;

    rad->radio->radio.spiWrite(RH_RF69_REG_5A_TESTPA1, RH_RF69_TESTPA1_NORMAL);
    rad->radio->radio.spiWrite(RH_RF69_REG_5C_TESTPA2, RH_RF69_TESTPA2_NORMAL);
    uint8_t opmode = rad->radio->radio.spiRead(RH_RF69_REG_01_OPMODE);
    opmode &= ~RH_RF69_OPMODE_MODE;
    opmode |= (RH_RF69_OPMODE_MODE_STDBY & RH_RF69_OPMODE_MODE);
    rad->radio->radio.spiWrite(RH_RF69_REG_01_OPMODE, opmode);
}

void radioRx(RadioObject *rad)
{
    rad->settings.modeReady = false;
    rad->settings.modeIdle = false;
    rad->radio->radio.spiWrite(RH_RF69_REG_5A_TESTPA1, RH_RF69_TESTPA1_NORMAL);
    rad->radio->radio.spiWrite(RH_RF69_REG_5C_TESTPA2, RH_RF69_TESTPA2_NORMAL);
    rad->radio->radio.spiWrite(RH_RF69_REG_25_DIOMAPPING1, RH_RF69_DIOMAPPING1_DIO0MAPPING_01); // Set interrupt line 0 PayloadReady
    uint8_t opmode = rad->radio->radio.spiRead(RH_RF69_REG_01_OPMODE);
    opmode &= ~RH_RF69_OPMODE_MODE;
    opmode |= (RH_RF69_OPMODE_MODE_RX & RH_RF69_OPMODE_MODE);
    rad->radio->radio.spiWrite(RH_RF69_REG_01_OPMODE, opmode);
}