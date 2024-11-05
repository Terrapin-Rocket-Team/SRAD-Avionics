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

    radio.setProperty(G_MODEM, P_MODEM_MOD_TYPE, 0x03);
    radio.setProperty(G_MODEM, P_MODEM_MAP_CONTROL, 0x00);
    // radio.setProperty(G_FREQ_CONTROL, P_FREQ_CONTROL_VCOCNT_RX_ADJ, 0xff);
    // radio.setProperty(G_MODEM, P_MODEM_MDM_CTRL, 0x00);
    // radio.setProperty(G_MODEM, P_MODEM_IF_CONTROL, 0x08);
    // uint8_t mIFFreq[3] = {0x03, 0xc0, 0x00};
    // radio.setProperty(G_MODEM, 3, P_MODEM_IF_FREQ3, mIFFreq);
    // radio.setProperty(G_MODEM, P_MODEM_DECIMATION_CFG_0, 0x20);
    // radio.setProperty(G_MODEM, P_MODEM_DECIMATION_CFG_1, 0xb0);
    // uint8_t mBCROSR[2] = {0x00, 0x5e};
    // radio.setProperty(G_MODEM, 2, P_MODEM_BCR_OSR2, mBCROSR);
    // uint8_t mBCRNCOOff[3] = {0x50, 0x76, 0x1a};
    // radio.setProperty(G_MODEM, 3, P_MODEM_BCR_NCO_OFFSET3, mBCRNCOOff);
    // uint8_t mBCRGain[2] = {0x07, 0xff};
    // radio.setProperty(G_MODEM, 2, P_MODEM_BCR_GAIN2, mBCRGain);
    // radio.setProperty(G_MODEM, P_MODEM_BCR_GEAR, 0x02);
    // radio.setProperty(G_MODEM, P_MODEM_BCR_MISC_1, 0x00);
    // radio.setProperty(G_MODEM, P_MODEM_BCR_MISC_0, 0x00);
    // radio.setProperty(G_MODEM, P_MODEM_AFC_GEAR, 0x00);
    // radio.setProperty(G_MODEM, P_MODEM_AFC_WAIT, 0x12);
    // uint8_t mAFCGain[2] = {0x80, 0x16};
    // radio.setProperty(G_MODEM, 2, P_MODEM_AFC_GAIN2, mAFCGain);
    // uint8_t mAFCLimiter[2] = {0x01, 0x76};
    // radio.setProperty(G_MODEM, 2, P_MODEM_AFC_LIMITER2, mAFCLimiter);
    // radio.setProperty(G_MODEM, P_MODEM_AFC_MISC, 0xe0);
    // radio.setProperty(G_MODEM, P_MODEM_AGC_CONTROL, 0xe2);
    // radio.setProperty(G_MODEM, P_MODEM_AGC_WINDOW_SIZE, 0x11);
    // radio.setProperty(G_MODEM, P_MODEM_AGC_RFPD_DECAY, 0x15);
    // radio.setProperty(G_MODEM, P_MODEM_AGC_IFPD_DECAY, 0x15);
    // radio.setProperty(G_MODEM, P_MODEM_OOK_CNT1, 0xa4);
    // radio.setProperty(G_MODEM, P_MODEM_OOK_MISC, 0x03);
    // radio.setProperty(G_MODEM, P_MODEM_RAW_SEARCH2, 0xd6);
    // radio.setProperty(G_MODEM, P_MODEM_RAW_CONTROL, 0x03);
    // uint8_t mRawEye[2] = {0x00, 0xde};
    // radio.setProperty(G_MODEM, 2, P_MODEM_RAW_EYE2, mRawEye);
    // radio.setProperty(G_MODEM, P_MODEM_RSSI_COMP, 0x40);
    // radio.setProperty(G_MODEM, P_MODEM_RSSI_CONTROL, 0x00);
    // radio.setProperty(G_MODEM, P_MODEM_RSSI_THRESH, 0xff);
    // radio.setProperty(G_MODEM, P_MODEM_RSSI_JUMP_THRESH, 0x0c);
    radio.setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG, 0x02);
    radio.setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG_STD_1, 0x14);
    radio.setProperty(G_PREAMBLE, P_PREAMBLE_CONFIG_STD_2, 0x08);
    radio.setProperty(G_SYNC, P_SYNC_CONFIG, 0x01);
    uint8_t pField1Length[2] = {0x00, 0x0A};
    radio.setProperty(G_PKT, 2, P_PKT_FIELD_1_LENGTH2, pField1Length);
    radio.setProperty(G_PKT, P_PKT_FIELD_1_CONFIG, 0x00);

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
        Serial.println("INTERRUPTS");
        for (int i = 0; i < 8; i++)
        {
            Serial.println(rIntArgs[i], BIN);
        }
        uint8_t startRX[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00};
        radio.sendCommandC(C_START_RX, 7, startRX);
        rxMode = true;
        radio.waitCTS();
        Serial.println("In receive mode");
    }

    uint8_t frrStates[4] = {0};
    radio.readFRRs(frrStates);
    if (millis() - timer > 2000)
    {
        timer = millis();
        uint8_t argsp[2] = {0};
        Serial.println("PH_STATUS");
        radio.sendCommandR(C_GET_PH_STATUS, 2, argsp);

        Serial.println(argsp[0], BIN);
        Serial.println(argsp[1], BIN);

        Serial.println("FIFO_INFO");
        uint8_t args[2] = {};
        uint8_t argst[1] = {0};
        radio.sendCommand(C_FIFO_INFO, 1, argst, 2, args);
        Serial.println(args[0]);
        Serial.println(args[1]);

        Serial.println("FRRs");
        Serial.println(frrStates[0], HEX);
        Serial.println(frrStates[1]);
        Serial.println(frrStates[2], BIN);
        Serial.println(frrStates[3], BIN);
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