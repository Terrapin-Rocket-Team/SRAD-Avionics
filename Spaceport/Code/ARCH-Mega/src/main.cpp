#include "Arduino.h"
#include "pin_defs.h"
#include "Timer.h"

#define MSG_SIZE_OVRD 200
#include "RadioMessage.h"

#define ADC_MAX 1023.

const uint32_t PWR_CHNS[] = {PWR_CH1, PWR_CH2, PWR_CH3, PWR_CH4, PWR_CH5, PWR_CH6};

HardwareSerial Serial1A(PA_10_R, PA_9_R);
Message m;
GSControl cmdIn;
Timer voltReadRate(1000);
Timer startCH1(1 * 60 * 60 * 1000, TIMER_ONCE);
char voltStr[50];
bool hasMessage = false;

void getVoltage(float &bat, float &rail, float &charge);
bool handler(char *cmd, uint16_t argc, char **argv);

void setup()
{
    Serial1A.begin(115200);

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

    delay(1000);
    digitalWrite(PWR_CH1, PWR_CH1_DEFAULT);
    delay(100);
    digitalWrite(PWR_CH2, PWR_CH2_DEFAULT);
    delay(100);
    digitalWrite(PWR_CH3, PWR_CH3_DEFAULT);
    delay(100);
    digitalWrite(PWR_CH4, PWR_CH4_DEFAULT);
    delay(100);
    digitalWrite(PWR_CH5, PWR_CH5_DEFAULT);
    delay(100);
    digitalWrite(PWR_CH6, PWR_CH6_DEFAULT);

    digitalWrite(STAT, HIGH);
}

void loop()
{
    if (Serial1A.available())
    {
        char c = Serial1A.read();
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
        Serial1A.write(res ? "ACK" : "NAK");
        Serial1A.write(0);
    }

    if (voltReadRate.evaluate())
    {
        digitalWrite(STAT, HIGH);
        // get current voltages
        float bat = 1, rail = 2, charge = 0;
        getVoltage(bat, rail, charge);
        // assemble data
        snprintf(voltStr, sizeof(voltStr), "BAT=%d,RAIL=%d,CHG=%d", (int)(bat * 1000), (int)(rail * 1000), (int)(charge * 1000));
        // write data
        Serial1A.write(voltStr);
        Serial1A.write('\n');
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