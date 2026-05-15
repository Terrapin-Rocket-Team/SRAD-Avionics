#include "Arduino.h"
#include "pin_defs.h"
#include "Timer.h"

#define MSG_SIZE_OVRD 200
#include "RadioMessage.h"

#define ADC_MAX 1023.

const int PWR_CHNS[] = {PWR_CH1, PWR_CH2, PWR_CH3, PWR_CH4, PWR_CH5, PWR_CH6};

HardwareSerial *s = nullptr;
Message m;
GSControl cmdIn;
Timer voltReadRate(1000);
Timer startCH1(1 * 60 * 60 * 1000, TIMER_ONCE);
char voltStr[100];
bool hasMessage = false;

void getVoltage(float &bat, float &rail, float &charge);
bool handler(char *cmd, uint16_t argc, char **argv);

void setup()
{
    Serial.begin(115200);
    s = (HardwareSerial *)&Serial;

    pinMode(BAT_VOLT, INPUT);
    pinMode(RAIL_VOLT, INPUT);
    pinMode(CHARGE_VOLT, INPUT);

    pinMode(STAT, OUTPUT);
    pinMode(PWR_CH1, OUTPUT);
    pinMode(PWR_CH2, OUTPUT);
    pinMode(PWR_CH3, OUTPUT);
    pinMode(PWR_CH4, OUTPUT);
    pinMode(PWR_CH5, OUTPUT);
    pinMode(PWR_CH6, OUTPUT);

    uint32_t ch1 = PA_2;
    uint32_t ch2 = PA_1;
    pinMode(ch1, OUTPUT);
    pinMode(ch2, OUTPUT);
    digitalWrite(ch1, HIGH);
    digitalWrite(ch2, HIGH);

    // digitalWrite(PWR_CH1, PWR_CH1_DEFAULT);
    // digitalWrite(PWR_CH2, PWR_CH2_DEFAULT);
    // digitalWrite(PWR_CH3, PWR_CH3_DEFAULT);
    // digitalWrite(PWR_CH4, PWR_CH4_DEFAULT);
    // digitalWrite(PWR_CH5, PWR_CH5_DEFAULT);
    // digitalWrite(PWR_CH6, PWR_CH6_DEFAULT);

    digitalWrite(STAT, HIGH);
    // digitalWrite(PWR_CH2, HIGH);
}

void loop()
{
    if (Serial.available())
    {
        char c = Serial.read();
        if (c != 0)
        {
            m.append(c);
        }
        else
        {
            hasMessage = true;
        }
    }

    if (hasMessage)
    {
        m.decode(&cmdIn);
        bool res = cmdIn.processCmd(handler);
        m.clear();
        hasMessage = false;
        Serial.write(res ? "ACK" : "NAK");
        Serial.write(0);
    }

    if (voltReadRate.evaluate())
    {
        digitalWrite(STAT, HIGH);
        // get current voltages
        float bat = 0, rail = 0, charge = 0;
        getVoltage(bat, rail, charge);
        // assemble data
        snprintf(voltStr, sizeof(voltStr), "BAT=%.2f,RAIL=%.2f,CHG=%.2f", bat, rail, charge);
        // write data
        Serial.write(voltStr);
        Serial.write(0);
        digitalWrite(STAT, LOW);
    }

    if (startCH1.evaluate())
    {
        digitalWrite(PWR_CH1, HIGH);
    }
}

void getVoltage(float &bat, float &rail, float &charge)
{
    // get adc reading as a percent of max value
    // then multiply by the max readable value of the voltage on the other end of the voltage divider
    bat = float(analogRead(BAT_VOLT)) / ADC_MAX * BAT_VOLT_MAX;
    rail = float(analogRead(RAIL_VOLT)) / ADC_MAX * RAIL_VOLT_MAX;
    charge = float(analogRead(CHARGE_VOLT)) / ADC_MAX * CHARGE_VOLT_MAX;
}

bool handler(char *cmd, uint16_t argc, char **argv)
{
    if (argc == 1)
    {
        uint8_t outputNum = atoi(cmd) - 1;

        if (strcmp(argv[0], "on") == 0)
        {
            digitalWrite(PWR_CHNS[outputNum], HIGH);
            return false;
        }
        if (strcmp(argv[0], "off") == 0)
        {
            digitalWrite(PWR_CHNS[outputNum], LOW);
            return false;
        }
    }
    return false;
}