#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

#define BUZZER 0

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK, // modulation
    DR_40k,    // data rate
    433e6,     // frequency (Hz)
    127,       // tx power (127 = ~20dBm)
    48,        // preamble length
    16,        // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
uint32_t timer = millis();
bool timer2 = false;
bool rxMode = false;

APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

APRSText testMessage(aprscfg, "RSSI test", "");

void beep(int d)
{
    digitalWrite(BUZZER, HIGH);
    delay(d);
    digitalWrite(BUZZER, LOW);
    delay(d);
}

void setup()
{
    Serial.begin(9600);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    if (!radio.begin())
    {
        Serial.println("Error: radio failed to begin");
        Serial.flush();
        while (1)
        {
            beep(1000);
        }
    }

    Serial.println("Radio began successfully");

    beep(100);
}

void loop()
{
    if (!rxMode)
    {
        Serial.println("Entering receive mode");
        // clear pending interrupts
        uint8_t cIntArgs[3] = {0, 0, 0};
        uint8_t rIntArgs[8] = {};
        radio.sendCommand(C_GET_INT_STATUS, 3, cIntArgs, 8, rIntArgs);
        // Serial.println("INTERRUPTS");
        // for (int i = 0; i < 8; i++)
        // {
        //     Serial.println(rIntArgs[i], BIN);
        // }
        uint8_t startRX[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00};
        radio.sendCommandC(C_START_RX, 7, startRX);
        rxMode = true;
        radio.waitCTS();
        // Serial.println("In receive mode");
    }

    uint8_t frrStates[4] = {0};
    radio.readFRRs(frrStates);
    if (millis() - timer > 2000)
    {
        timer = millis();
        uint8_t argsp[2] = {0};
        //Serial.println("PH_STATUS");
        radio.sendCommandR(C_GET_PH_STATUS, 2, argsp);

        // Serial.println(argsp[0], BIN);
        // Serial.println(argsp[1], BIN);

        // Serial.println("FIFO_INFO");
        uint8_t args[2] = {};
        uint8_t argst[1] = {0};
        radio.sendCommand(C_FIFO_INFO, 1, argst, 2, args);
        // Serial.println(args[0]);
        // Serial.println(args[1]);

        // Serial.println("FRRs");
        // Serial.println(frrStates[0], HEX);
        // Serial.println(frrStates[1]);
        // Serial.println(frrStates[2], BIN);
        // Serial.println(frrStates[3], BIN);
    }

    if (frrStates[1] > 0)
    {
        Serial.println("Detected");
        Serial.println(frrStates[1]);
        Serial.println(frrStates[2], BIN);
        Serial.println(frrStates[3], BIN);
        Serial.flush();
        while (1)
        {
        }
    }

    if (frrStates[2] > 0)
    {
        if ((frrStates[2] & 0b1000000) != 0)
        {
            Serial.println(frrStates[2], BIN);
            Serial.println("Packet received, RSSI latch detected");
            Serial.print("RSSI: ");
            Serial.println(frrStates[1]);

            uint8_t argsp[2] = {0};
            Serial.println("PH_STATUS");
            radio.sendCommandR(C_GET_PH_STATUS, 2, argsp);
            Serial.println(argsp[0], BIN);
            Serial.println(argsp[1], BIN);

            rxMode = false;
            uint8_t changeState[1] = {3};
            radio.sendCommandC(C_CHANGE_STATE, 1, changeState);
        }
    }

    if (frrStates[0] == 3 && rxMode == true)
    {

        rxMode = false;
        uint8_t changeState[1] = {3};
        radio.sendCommandC(C_CHANGE_STATE, 1, changeState);
    }
    // need to call as fast as possible every loop
    // radio.update();
}