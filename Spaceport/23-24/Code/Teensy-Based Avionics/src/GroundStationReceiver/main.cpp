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

// 1, 2 are for live radio, 3 is for telemetry, 4 is for input
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

void radioIdle(LiveRadioObject *rad);
void radioRx(LiveRadioObject *rad);
void readLiveRadio(LiveRadioObject *rad);

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
};

struct LiveRadioObject
{
    Live::RFM69HCW *radio;
    const int packetSize;
    byte *buffer;
    int bufftop;        // index of the next byte to be written
    int buffbot;        // index of the next byte to be read
    LiveSettings settings;
};

// just for sending info
int bufftop4 = 0;
int buffbot4 = 0;

Live::APRSConfig config1 = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
Live::RadioSettings settings1 = {915.0, false, true, &hardware_spi, 10, 15, 14, 7, 8, 9};
Live::RFM69HCW radio1(&settings1, &config1);

Live::APRSConfig config2 = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
Live::RadioSettings settings2 = {915.0, false, true, &hardware_spi, 10, 15, 14, 7, 8, 9};
Live::RFM69HCW radio2(&settings1, &config2);

// Let radio3 be the telemetry receiver
APRSHeader header = {"KC3UTM", "APRS", "WIDE1-1", '/', 'o'};
APRSTelemMsg msg3(header);
APRSCmdMsg cmd3(header);
RadioSettings settings3 = {433.775, 0x02, 0x01, &hardware_spi, 10, 31, 32};
RFM69HCW radio3(&settings3);

LiveSettings lset1 = {false, false, true, 0, false, false};
LiveSettings lset2 = {false, false, true, 0, false, false};
LiveRadioObject r1 = {&radio1, PACKET1_SIZE, buff1, 0, 0, lset1};
LiveRadioObject r2 = {&radio2, PACKET2_SIZE, buff2, 0, 0, lset2};
RadioObject r3 = {&msg3, &cmd3, &radio3, PACKET3_SIZE, buff3, 0, 0};

// make an array of pointers to the radio objects
void *radios[] = {&r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r1, &r2, &r3};

void setup()
{
    delay(2000); // Delay to allow the serial monitor to connect
    if (!radio1.begin())
        Serial.println("Radio1 failed to initialize");
    else
        Serial.println("Radio1 initialized");

    if (!radio2.begin())
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

    if (radio3.update())
    {
        if (radio3.dequeueReceive(&msg3))
        {
            char buffer[80];
            aprsToSerial::encodeAPRSForSerial(msg3, buffer, 80, radio3.RSSI());
            
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
void radioIdle(LiveRadioObject *rad)
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

void radioRx(LiveRadioObject *rad)
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

void readLiveRadio(LiveRadioObject *rad)
{

    using namespace Live;

    // Refill fifo here
    if (rad->radio->FifoNotEmpty() && rad->settings.hasTransmission)
    {
        // Start spi transaction
        rad->radio->settings.spi->beginTransaction();

        // Select the radio
        digitalWrite(rad->radio->settings.cs, LOW);

        // Send the fifo address with the write mask off
        rad->radio->settings.spi->transfer(RH_RF69_REG_00_FIFO);

        // data
        while (rad->settings.pos < MSG_SIZE && rad->radio->FifoNotEmpty())
        {
            Serial.write(rad->radio->settings.spi->transfer(0));
            rad->settings.pos++;
        }

        // reset msgLen and toAddr, end transaction, and clear fifo through entering idle mode
        digitalWrite(rad->radio->settings.cs, HIGH);
        rad->radio->settings.spi->endTransaction();

        if (rad->settings.pos == MSG_SIZE)
        {
            // Serial.println("finished");
            rad->settings.pos = 0;
            rad->settings.hasTransmission = false;
            radioIdle(rad);
        }
    }

    // Start a new transmission
    if (rad->radio->FifoNotEmpty() && !(rad->settings.hasTransmission) && !(rad->settings.modeIdle) && (rad->settings.modeReady))
    {
        // Serial.println("started receiving");
        // timer = millis();
        // Start spi transaction
        rad->radio->settings.spi->beginTransaction();
        // Select the radio
        digitalWrite(rad->radio->settings.cs, LOW);

        // Send the fifo address with the write mask off
        rad->radio->settings.spi->transfer(RH_RF69_REG_00_FIFO);
        // rad->settings.pos = 0;

        // while (rad->settings.pos < MSG_SIZE)
        // {
        //   if (rad->radio->FifoNotEmpty())
        //   {
        //     Serial.write(settings.spi->transfer(0));
        //     rad->settings.pos++;
        //   }
        // }

        if (!(rad->settings.hasL))
        {
            rad->settings.hasL = rad->radio->settings.spi->transfer(0) == SYNC1;
        }

        if (!(rad->settings.hasA) && rad->settings.hasL && rad->radio->FifoNotEmpty())
        {
            rad->settings.hasL = rad->settings.hasA = rad->radio->settings.spi->transfer(0) == SYNC2;
            if (!(rad->settings.hasA))
            {
                // radioIdle();
            }
        }

        // if found sync bytes
        if (rad->settings.hasL && rad->settings.hasA)
        {
            // set up for receiving the message
            rad->settings.hasTransmission = true;
            rad->settings.hasL = rad->settings.hasA = false;

            while (rad->settings.pos < MSG_SIZE && rad->radio->FifoNotEmpty())
            {
                Serial.write(rad->radio->settings.spi->transfer(0));
                rad->settings.pos++;
            }
        }
        // reset msgLen and toAddr, end transaction, and clear fifo through entering idle mode
        digitalWrite(rad->radio->settings.cs, HIGH);
        rad->radio->settings.spi->endTransaction();

        if (rad->settings.pos == MSG_SIZE)
        {
            // Serial.println("finished 1");
            rad->settings.pos = 0;
            rad->settings.hasTransmission = false;
            radioIdle(rad);
        }
    }

    if ((rad->radio->radio.spiRead(RH_RF69_REG_27_IRQFLAGS1) & RH_RF69_IRQFLAGS1_MODEREADY) && !(rad->settings.modeReady))
    {
        rad->settings.modeReady = true;
        if (rad->settings.modeIdle)
            radioRx(rad);
    }
}   