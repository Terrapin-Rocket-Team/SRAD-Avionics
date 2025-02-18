#include "Arduino.h"
#include "RadioMessage.h"
#include "Si4463.h"

#define BUZZER 0

Si4463HardwareConfig hwcfg = {
    MOD_2FSK, // modulation
    DR_40k,   // data rate
    433e6,    // frequency (Hz)
    127,      // tx power (127 = ~20dBm)
    48,       // preamble length
    16,       // required received valid preamble
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

    radio.setProperty(G_MODEM, P_MODEM_MOD_TYPE, 0x03);
    radio.setProperty(G_MODEM, P_MODEM_MAP_CONTROL, 0x00);
    uint8_t mdataRate[3] = {0x00, 0x4e, 0x20};
    radio.setProperty(G_MODEM, 3, P_MODEM_DATA_RATE3, mdataRate);
    uint8_t mTXNCOMode[4] = {0x04, 0x2d, 0xc6, 0xc0};
    radio.setProperty(G_MODEM, 4, P_MODEM_TX_NCO_MODE4, mTXNCOMode);
    uint8_t mFreqDev[3] = {0x00, 0x00, 0x39};
    radio.setProperty(G_MODEM, 3, P_MODEM_FREQ_DEV3, mFreqDev);
    radio.setProperty(G_MODEM, P_MODEM_TX_RAMP_DELAY, 0x01);
    radio.setProperty(G_PREAMBLE, P_PREAMBLE_TX_LENGTH, 0x0a);
    radio.setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG, 0x12);
    radio.setProperty(G_SYNC, P_SYNC_CONFIG, 0x01);
    radio.setProperty(G_PKT, P_PKT_LEN, 0x00);
    uint8_t pField1Length[2] = {0x00, 0x0A};
    radio.setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, pField1Length);
    radio.setProperty(G_PKT, P_PKT_FIELD_1_CONFIG, 0x00);

    Serial.println("Radio began successfully");

    beep(100);
}

void loop()
{
    if (millis() - timer > 5000)
    {
        timer = millis();
        timer2 = true;
        Serial.println("Sending message");
        uint8_t writeTXFIFO[10] = {0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49, 0x49};
        radio.sendCommandC(C_WRITE_TX_FIFO, 10, writeTXFIFO);
        uint8_t startTX[4] = {0x00, 0x30, 0x00, 0x00};
        radio.sendCommandC(C_START_TX, 4, startTX);
    }
    // if (timer2 && millis() - timer > 1000)
    // {
    //     timer2 = false;
    //     uint8_t changeState[1] = {3};
    //     radio.sendCommandC(C_CHANGE_STATE, 1, changeState);
    // }
    // need to call as fast as possible every loop
    // radio.update();
}